#include <math.h>
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

// --- 実行ファイルのディレクトリを取得（OS別実装） ---
#if defined(_WIN32)
#include <windows.h>
static std::filesystem::path GetExeDir()
{
    std::wstring buf(1024, L'\0');
    DWORD len = ::GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    if (len >= buf.size())
    {
        // バッファ拡張
        buf.resize(len + 1);
        len = ::GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    }
    buf.resize(len);
    std::filesystem::path exePath(buf);
    return exePath.parent_path();
}
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
static std::filesystem::path GetExeDir()
{
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size); // size取得
    std::vector<char> buf(size);
    if (_NSGetExecutablePath(buf.data(), &size) != 0)
    {
        return std::filesystem::current_path(); // フォールバック
    }
    std::filesystem::path exePath = std::filesystem::canonical(std::filesystem::path(buf.data()));
    return exePath.parent_path();
}
#else // Linux/Unix
#include <unistd.h>
static std::filesystem::path GetExeDir()
{
    std::vector<char> buf(1024);
    ssize_t len = ::readlink("/proc/self/exe", buf.data(), buf.size() - 1);
    if (len == -1)
    {
        return std::filesystem::current_path(); // フォールバック
    }
    buf[len] = '\0';
    std::filesystem::path exePath = std::filesystem::canonical(std::filesystem::path(buf.data()));
    return exePath.parent_path();
}
#endif

int main(int, char *[])
{
    vtkNew<vtkNamedColors> nc;

    // Spiral tube parameters
    unsigned int nV = 256;       // No. of vertices
    unsigned int nCyc = 5;       // No. of spiral cycles
    double rT1 = 0.1, rT2 = 0.5; // Start/end tube radii
    double rS = 2;               // Spiral radius
    double h = 10;               // Height
    unsigned int nTv = 8;        // No. of sides for each tube cross-section

    // Create points for the helix centerline.
    vtkNew<vtkPoints> points;
    for (unsigned int i = 0; i < nV; i++)
    {
        // Helical coordinates
        double vX = rS * cos(2 * vtkMath::Pi() * nCyc * i / (nV - 1));
        double vY = rS * sin(2 * vtkMath::Pi() * nCyc * i / (nV - 1));
        double vZ = h * static_cast<double>(i) / static_cast<double>(nV);
        points->InsertPoint(i, vX, vY, vZ);
    }

    // Polyline cells
    vtkNew<vtkCellArray> lines;
    lines->InsertNextCell(nV);
    for (unsigned int i = 0; i < nV; i++)
    {
        lines->InsertCellPoint(i);
    }

    vtkNew<vtkPolyData> polyData;
    polyData->SetPoints(points);
    polyData->SetLines(lines);

    // Per-point tube radius via sine function
    vtkNew<vtkDoubleArray> tubeRadius;
    tubeRadius->SetName("TubeRadius");
    tubeRadius->SetNumberOfTuples(nV);
    for (unsigned int i = 0; i < nV; i++)
    {
        tubeRadius->SetTuple1(
            i, rT1 + (rT2 - rT1) * sin(vtkMath::Pi() * i / (nV - 1)));
    }
    polyData->GetPointData()->AddArray(tubeRadius);
    polyData->GetPointData()->SetActiveScalars("TubeRadius");

    // RGB color array (blue -> red) for visualization only (not saved to STL)
    vtkNew<vtkUnsignedCharArray> colors;
    colors->SetName("Colors");
    colors->SetNumberOfComponents(3);
    colors->SetNumberOfTuples(nV);
    for (unsigned int i = 0; i < nV; i++)
    {
        colors->InsertTuple3(i,
                             static_cast<unsigned char>(255.0 * i / (nV - 1)),
                             0,
                             static_cast<unsigned char>(255.0 * (nV - 1 - i) / (nV - 1)));
    }
    polyData->GetPointData()->AddArray(colors);

    // Tube from centerline
    vtkNew<vtkTubeFilter> tube;
    tube->SetInputData(polyData);
    tube->SetNumberOfSides(nTv);
    tube->SetVaryRadiusToVaryRadiusByAbsoluteScalar(); // Use "TubeRadius"

    // ======== STL 書き出し：exeの場所から2階層上の output/ に出力 ========
    // 実行ファイルのディレクトリ
    std::filesystem::path exeDir = GetExeDir();
    // 2階層上の output ディレクトリ
    std::filesystem::path outDir = exeDir / ".." / ".." / "output";
    outDir = std::filesystem::weakly_canonical(outDir);
    std::error_code ec;
    std::filesystem::create_directories(outDir, ec); // 既存ならOK

    std::filesystem::path outFile = outDir / "tube.stl";

    // 三角形化（STLは三角形面）
    vtkNew<vtkTriangleFilter> tri;
    tri->SetInputConnection(tube->GetOutputPort());

    vtkNew<vtkSTLWriter> stlWriter;
    stlWriter->SetInputConnection(tri->GetOutputPort());
    // 注意: 非ASCIIパスの場合、環境によってはUTF-8→ワイドの扱いが必要になることがある
    const std::string outFileStr = outFile.string();
    stlWriter->SetFileName(outFileStr.c_str());
    // stlWriter->SetFileTypeToASCII(); // 必要なら ASCII で
    stlWriter->Write();
    // ======== ここまで ========

    // 可視化（色は描画用のみ）
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(tube->GetOutputPort()); // tri でもOK
    mapper->ScalarVisibilityOn();
    mapper->SetScalarModeToUsePointFieldData();
    mapper->SelectColorArray("Colors");

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    vtkNew<vtkRenderer> renderer;
    renderer->AddActor(actor);
    renderer->SetBackground(nc->GetColor3d("SteelBlue").GetData());

    // Oblique view
    renderer->GetActiveCamera()->Azimuth(30);
    renderer->GetActiveCamera()->Elevation(30);
    renderer->ResetCamera();

    vtkNew<vtkRenderWindow> renWin;
    vtkNew<vtkRenderWindowInteractor> iren;
    iren->SetRenderWindow(renWin);
    renWin->AddRenderer(renderer);
    renWin->SetSize(500, 500);
    renWin->SetWindowName("TubeFromCenterline (with STL export)");
    renWin->Render();

    vtkNew<vtkInteractorStyleTrackballCamera> style;
    iren->SetInteractorStyle(style);
    iren->Start();

    return EXIT_SUCCESS;
}
