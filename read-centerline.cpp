#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cctype>
#include <limits>
#include <algorithm>
#include <cstdlib>
#include <cerrno>

struct Point3D
{
    double x, y, z;
    // if there is a radius column in centerline file(*.csv)
    double radius = std::numeric_limits<double>::quiet_NaN();
    bool has_radius = false;
};

static bool is_number(const std::string &s)
{
    if (s.empty())
        return false;
    char *end = nullptr;
    errno = 0;
    (void)std::strtod(s.c_str(), &end);
    if (errno != 0)
        return false;
    while (end && std::isspace(static_cast<unsigned char>(*end)))
        ++end;
    return end && *end == '\0';
}

static std::vector<std::string> tokenize_relaxed(std::string line)
{
    for (char &c : line)
    {
        if (c == ',' || c == ';' || c == '\t')
            c = ' ';
    }
    std::istringstream iss(line);
    std::vector<std::string> toks;
    std::string tok;
    while (iss >> tok)
        toks.push_back(tok);
    return toks;
}

static bool parse_xyzr_from_line(const std::string &line, Point3D &out)
{
    std::string trimmed = line;

    auto notspace = [](int ch)
    { return !std::isspace(static_cast<unsigned char>(ch)); };
    trimmed.erase(trimmed.begin(),
                  std::find_if(trimmed.begin(), trimmed.end(), notspace));
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), notspace).base(),
                  trimmed.end());

    if (trimmed.empty())
        return false;
    if (trimmed[0] == '#')
        return false;

    auto toks = tokenize_relaxed(trimmed);
    if (toks.size() < 3)
        return false;

    if (is_number(toks[0]) && is_number(toks[1]) && is_number(toks[2]))
    {
        out.x = std::stod(toks[0]);
        out.y = std::stod(toks[1]);
        out.z = std::stod(toks[2]);

        if (toks.size() >= 4 && is_number(toks[3]))
        {
            out.radius = std::stod(toks[3]);
            out.has_radius = true;
        }
        else
        {
            out.radius = std::numeric_limits<double>::quiet_NaN();
            out.has_radius = false;
        }
        return true;
    }
    return false;
}

std::vector<Point3D> load_centerline_csv(const std::string &path, bool &had_header)
{
    std::ifstream ifs(path);
    if (!ifs)
    {
        throw std::runtime_error("can't open file: " + path);
    }

    std::vector<Point3D> pts;
    had_header = false;

    std::string line;
    bool first_meaningful_seen = false;

    while (std::getline(ifs, line))
    {
        Point3D p;
        bool ok = parse_xyzr_from_line(line, p);
        if (!first_meaningful_seen)
        {
            auto toks = tokenize_relaxed(line);
            if (!ok && toks.size() >= 3 &&
                (!is_number(toks[0]) || !is_number(toks[1]) || !is_number(toks[2])))
            {
                had_header = true;
            }
            if (!ok)
            {
                first_meaningful_seen = true;
                continue;
            }
            first_meaningful_seen = true;
        }

        if (ok)
            pts.push_back(p);
    }
    return pts;
}

int main(int argc, char **argv)
{
    std::cout << "START: program entered main()\n";
    std::cout.flush();

    if (argc < 2)
    {
        std::cerr << "How To Use. put input data at: " << argv[0] << " input.csv\n";
        std::cout << "and excute from terminal()\n";
        return 1;
    }

    try
    {
        bool had_header = false;
        auto pts = load_centerline_csv(argv[1], had_header);

        std::cout << "num of centerline points: " << pts.size();
        if (had_header)
            std::cout << "(header)";
        std::cout << "\n";

        if (!pts.empty())
        {
            double xmin = std::numeric_limits<double>::infinity();
            double ymin = std::numeric_limits<double>::infinity();
            double zmin = std::numeric_limits<double>::infinity();
            double xmax = -std::numeric_limits<double>::infinity();
            double ymax = -std::numeric_limits<double>::infinity();
            double zmax = -std::numeric_limits<double>::infinity();

            for (const auto &p : pts)
            {
                xmin = std::min(xmin, p.x);
                xmax = std::max(xmax, p.x);
                ymin = std::min(ymin, p.y);
                ymax = std::max(ymax, p.y);
                zmin = std::min(zmin, p.z);
                zmax = std::max(zmax, p.z);
            }

            std::cout << "AABB: ["
                      << xmin << ", " << ymin << ", " << zmin << "] - ["
                      << xmax << ", " << ymax << ", " << zmax << "]\n";

            std::cout << "start point: (" << pts.front().x << ", " << pts.front().y << ", " << pts.front().z << ")\n";
            std::cout << "end point: (" << pts.back().x << ", " << pts.back().y << ", " << pts.back().z << ")\n";

            double rmin = std::numeric_limits<double>::infinity();
            double rmax = -std::numeric_limits<double>::infinity();
            long double rsum = 0.0L;
            std::size_t rcount = 0;

            for (const auto &p : pts)
            {
                if (p.has_radius)
                {
                    rmin = std::min(rmin, p.radius);
                    rmax = std::max(rmax, p.radius);
                    rsum += p.radius;
                    ++rcount;
                }
            }

            if (rcount > 0)
            {
                double rmean = static_cast<double>(rsum / static_cast<long double>(rcount));
                std::cout << "radius stats (from " << rcount << " points): "
                          << "min=" << rmin << ", max=" << rmax << ", mean=" << rmean << "\n";
            }
        }
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}