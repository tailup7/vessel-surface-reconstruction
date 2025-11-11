#include <math.h>
#include <fstream>
#include <sstream>
#include <iostream>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMath.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTubeFilter.h>
#include <vtkUnsignedCharArray.h>
#include <vtkTriangleFilter.h>
#include <vtkSTLWriter.h>

#include <filesystem>
#include <string>
#include <vector>

// --- Get executable directory (OS dependent) ---
#if defined(_WIN32)
#include <windows.h>
#include <commdlg.h>

static std::filesystem::path GetExeDir()
{
    std::wstring buf(1024, L'\0');
    DWORD len = ::GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    if (len >= buf.size())
    {
        buf.resize(len + 1);
        len = ::GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    }
    buf.resize(len);
    std::filesystem::path exePath(buf);
    return exePath.parent_path();
}

// Windows: open-file dialog for CSV
static std::filesystem::path OpenCsvFileDialog()
{
    wchar_t szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;

    std::wstring initialDir = GetExeDir().wstring();
    ofn.lpstrInitialDir = initialDir.c_str();

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn) == TRUE)
    {
        return std::filesystem::path(szFile);
    }
    return std::filesystem::path(); // empty = canceled
}

#elif defined(__APPLE__)
#include <mach-o/dyld.h>

static std::filesystem::path GetExeDir()
{
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buf(size);
    if (_NSGetExecutablePath(buf.data(), &size) != 0)
    {
        return std::filesystem::current_path();
    }
    std::filesystem::path exePath = std::filesystem::canonical(std::filesystem::path(buf.data()));
    return exePath.parent_path();
}

// Simple text-based "dialog" for macOS (replace with real GUI if needed)
static std::filesystem::path OpenCsvFileDialog()
{
    std::cout << "Enter CSV file path: " << std::flush;
    std::string path;
    if (!std::getline(std::cin, path))
    {
        return std::filesystem::path();
    }
    if (path.empty())
    {
        return std::filesystem::path();
    }
    return std::filesystem::path(path);
}

#else // Linux/Unix
#include <unistd.h>

static std::filesystem::path GetExeDir()
{
    std::vector<char> buf(1024);
    ssize_t len = ::readlink("/proc/self/exe", buf.data(), buf.size() - 1);
    if (len == -1)
    {
        return std::filesystem::current_path();
    }
    buf[len] = '\0';
    std::filesystem::path exePath = std::filesystem::canonical(std::filesystem::path(buf.data()));
    return exePath.parent_path();
}

// Simple text-based "dialog" for Linux (replace with real GUI if needed)
static std::filesystem::path OpenCsvFileDialog()
{
    std::cout << "Enter CSV file path: " << std::flush;
    std::string path;
    if (!std::getline(std::cin, path))
    {
        return std::filesystem::path();
    }
    if (path.empty())
    {
        return std::filesystem::path();
    }
    return std::filesystem::path(path);
}
#endif

// --- Load centerline points from CSV ---
// Expected format: each line "x,y,z" (3 columns, comma-separated)
static bool LoadCenterlineFromCsv(const std::filesystem::path &csvPath, vtkPoints *points)
{
    std::ifstream ifs(csvPath);
    if (!ifs)
    {
        std::cerr << "Failed to open CSV: " << csvPath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(ifs, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::stringstream ss(line);
        std::string sx, sy, sz;
        if (!std::getline(ss, sx, ',')) continue;
        if (!std::getline(ss, sy, ',')) continue;
        if (!std::getline(ss, sz, ',')) continue;

        try
        {
            double x = std::stod(sx);
            double y = std::stod(sy);
            double z = std::stod(sz);
            points->InsertNextPoint(x, y, z);
        }
        catch (const std::exception &)
        {
            // Ignore non-numeric rows (e.g. header)
            continue;
        }
    }

    if (points->GetNumberOfPoints() < 2)
    {
        std::cerr << "Valid point count is less than 2 (not enough for a centerline)." << std::endl;
        return false;
    }

    return true;
}

int main(int, char *[])
{
    vtkNew<vtkNamedColors> nc;

    // 1) Select CSV file via GUI / simple dialog
    std::filesystem::path csvPath = OpenCsvFileDialog();
    if (csvPath.empty())
    {
        std::cerr << "File selection canceled." << std::endl;
        return EXIT_FAILURE;
    }

    // 2) Load centerline points from CSV
    vtkNew<vtkPoints> points;
    if (!LoadCenterlineFromCsv(csvPath, points))
    {
        return EXIT_FAILURE;
    }

    const vtkIdType nV = points->GetNumberOfPoints();
    std::cout << "Loaded point count: " << nV << std::endl;

    // 3) Build polyline from points
    vtkNew<vtkCellArray> lines;
    lines->InsertNextCell(nV);
    for (vtkIdType i = 0; i < nV; ++i)
    {
        lines->InsertCellPoint(i);
    }

    vtkNew<vtkPolyData> polyData;
    polyData->SetPoints(points);
    polyData->SetLines(lines);

    // 4) Color (blue -> red, visualization only)
    vtkNew<vtkUnsignedCharArray> colors;
    colors->SetName("Colors");
    colors->SetNumberOfComponents(3);
    colors->SetNumberOfTuples(nV);

    for (vtkIdType i = 0; i < nV; ++i)
    {
        double t = (nV > 1) ? static_cast<double>(i) / static_cast<double>(nV - 1) : 0.0;
        unsigned char r = static_cast<unsigned char>(255.0 * t);
        unsigned char b = static_cast<unsigned char>(255.0 * (1.0 - t));
        unsigned char tuple[3] = { r, 0, b };
        colors->SetTypedTuple(i, tuple);
    }

    polyData->GetPointData()->AddArray(colors);

    // 5) TubeFilter: constant radius 0.8
    const double tubeRadius = 0.8;
    const unsigned int nTv = 8; // number of sides of tube cross-section

    vtkNew<vtkTubeFilter> tube;
    tube->SetInputData(polyData);
    tube->SetNumberOfSides(static_cast<int>(nTv));
    tube->SetRadius(tubeRadius);
    // Default: radius is constant, so no need to enable VaryRadius

    // 6) STL output: "output/tube.stl" two levels above exe dir
    std::filesystem::path exeDir = GetExeDir();
    std::filesystem::path outDir = exeDir / ".." / ".." / "output";
    std::error_code ec;
    outDir = std::filesystem::weakly_canonical(outDir, ec);
    std::filesystem::create_directories(outDir, ec);

    std::filesystem::path outFile = outDir / "tube.stl";

    vtkNew<vtkTriangleFilter> tri;
    tri->SetInputConnection(tube->GetOutputPort());

    vtkNew<vtkSTLWriter> stlWriter;
    stlWriter->SetInputConnection(tri->GetOutputPort());
    const std::string outFileStr = outFile.string();
    stlWriter->SetFileName(outFileStr.c_str());
    // stlWriter->SetFileTypeToASCII(); // Uncomment if you prefer ASCII STL
    stlWriter->Write();

    std::cout << "STL saved to: " << outFileStr << std::endl;

    // 7) Visualization
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(tube->GetOutputPort()); // or tri->GetOutputPort()
    mapper->ScalarVisibilityOn();
    mapper->SetScalarModeToUsePointFieldData();
    mapper->SelectColorArray("Colors");

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    vtkNew<vtkRenderer> renderer;
    renderer->AddActor(actor);
    renderer->SetBackground(nc->GetColor3d("SteelBlue").GetData());

    renderer->GetActiveCamera()->Azimuth(30);
    renderer->GetActiveCamera()->Elevation(30);
    renderer->ResetCamera();

    vtkNew<vtkRenderWindow> renWin;
    vtkNew<vtkRenderWindowInteractor> iren;
    iren->SetRenderWindow(renWin);
    renWin->AddRenderer(renderer);
    renWin->SetSize(500, 500);
    renWin->SetWindowName("TubeFromCenterline (CSV, constant radius)");
    renWin->Render();

    vtkNew<vtkInteractorStyleTrackballCamera> style;
    iren->SetInteractorStyle(style);
    iren->Start();

    return EXIT_SUCCESS;
}

