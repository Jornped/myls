#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <iomanip>
#include <sstream>
#include <cmath>

using namespace std;

// Константы цветов
const string RESET = "\033[0m";
const string DIR_COLOR = "\033[1;34m";     // Жирный синий (Директории)
const string EXEC_COLOR = "\033[1;32m";    // Жирный зеленый (Исполняемые)
const string SYMLINK_COLOR = "\033[1;36m"; // Жирный голубой (Симлинки)

struct Options
{
    bool l = false;
    bool r = false;
    bool h = false;
    bool useColors = false;
    string dirPath = ".";
};

struct FileInfo
{
    string name;
    struct stat st;
};

string colorizeName(const string &name, mode_t mode)
{
    if (S_ISDIR(mode))
    {
        return DIR_COLOR + name + RESET;
    }
    if (S_ISLNK(mode))
    {
        return SYMLINK_COLOR + name + RESET;
    }
    if ((mode & S_IXUSR) || (mode & S_IXGRP) || (mode & S_IXOTH))
    {
        return EXEC_COLOR + name + RESET;
    }

    return name;
}

string getPermissions(mode_t mode)
{
    string p = "----------";
    if (S_ISDIR(mode))
        p[0] = 'd';
    else if (S_ISLNK(mode))
        p[0] = 'l';
    else if (S_ISCHR(mode))
        p[0] = 'c';
    else if (S_ISBLK(mode))
        p[0] = 'b';
    else if (S_ISFIFO(mode))
        p[0] = 'p';
    else if (S_ISSOCK(mode))
        p[0] = 's';

    if (mode & S_IRUSR)
        p[1] = 'r';
    if (mode & S_IWUSR)
        p[2] = 'w';
    if (mode & S_IXUSR)
        p[3] = 'x';
    if (mode & S_IRGRP)
        p[4] = 'r';
    if (mode & S_IWGRP)
        p[5] = 'w';
    if (mode & S_IXGRP)
        p[6] = 'x';
    if (mode & S_IROTH)
        p[7] = 'r';
    if (mode & S_IWOTH)
        p[8] = 'w';
    if (mode & S_IXOTH)
        p[9] = 'x';
    return p;
}

string getHumanReadableSize(off_t bytes)
{
    if (bytes < 1024)
        return to_string(bytes);

    const char *units[] = {"K", "M", "G", "T"};
    int unit_index = -1;
    double size = bytes;

    while (size >= 1024 && unit_index < 3)
    {
        size /= 1024;
        unit_index++;
    }

    ostringstream oss;
    oss << fixed;
    if (size < 10.0 && fmod(size, 1.0) >= 0.05)
    {
        oss << setprecision(1) << size;
    }
    else
    {
        oss << setprecision(0) << size;
    }
    return oss.str() + units[unit_index];
}

string formatTime(time_t mtime)
{
    char timeBuf[64];
    struct tm *timeinfo = localtime(&mtime);
    strftime(timeBuf, sizeof(timeBuf), "%e %b %H:%M", timeinfo);
    return string(timeBuf);
}

string getOwnerName(uid_t uid)
{
    struct passwd *pw = getpwuid(uid);
    return pw ? pw->pw_name : to_string(uid);
}

string getGroupName(gid_t gid)
{
    struct group *gr = getgrgid(gid);
    return gr ? gr->gr_name : to_string(gid);
}

Options parseArguments(int argc, char *argv[])
{
    Options opts;
    int opt;
    while ((opt = getopt(argc, argv, "+lrh")) != -1)
    {
        switch (opt)
        {
        case 'l':
            opts.l = true;
            break;
        case 'r':
            opts.r = true;
            break;
        case 'h':
            opts.h = true;
            break;
        default:
            cerr << "Usage: " << argv[0] << " [-l] [-r] [-h] [directory]\n";
            exit(1);
        }
    }
    if (optind < argc - 1)
    {
        cerr << "Error: The directory path must be the last argument.\n";
        cerr << "Usage: " << argv[0] << " [-l] [-r] [-h] [directory]\n";
        exit(1);
    }
    if (optind < argc)
    {
        opts.dirPath = argv[optind];
    }
    return opts;
}

vector<FileInfo> readDirectory(const string &dirPath)
{
    vector<FileInfo> files;
    DIR *dir = opendir(dirPath.c_str());
    if (!dir)
    {
        perror(("ls: cannot get access to '" + dirPath + "'").c_str());
        return files;
    }

    string baseDir = dirPath;
    if (!baseDir.empty() && baseDir.back() != '/')
    {
        baseDir += '/';
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        string name = entry->d_name;
        if (name[0] == '.')
            continue;

        string fullPath = baseDir + name;
        struct stat fileStat;

        if (lstat(fullPath.c_str(), &fileStat) == -1)
        {
            perror(("ls: " + name).c_str());
            continue;
        }

        if (name.find(' ') != string::npos)
        {
            name = "'" + name + "'";
        }

        files.push_back({name, fileStat});
    }
    closedir(dir);
    return files;
}

void printShort(const vector<FileInfo> &files, const Options &opts)
{
    for (const auto &f : files)
    {
        string displayName = f.name;
        if (opts.useColors)
        {
            displayName = colorizeName(f.name, f.st.st_mode);
        }
        cout << displayName << "  ";
    }
    cout << "\n";
}

struct FormattedRow
{
    string perms, links, owner, group, size, timeStr, name;
};

void printLong(const vector<FileInfo> &files, const Options &opts)
{
    vector<FormattedRow> rows;
    rows.reserve(files.size());

    size_t max_links = 0, max_owner = 0, max_group = 0, max_size = 0;
    long long total_blocks = 0;

    for (const auto &f : files)
    {
        FormattedRow row;
        row.perms = getPermissions(f.st.st_mode);
        row.links = to_string(f.st.st_nlink);
        row.owner = getOwnerName(f.st.st_uid);
        row.group = getGroupName(f.st.st_gid);
        row.size = opts.h ? getHumanReadableSize(f.st.st_size) : to_string(f.st.st_size);
        row.timeStr = formatTime(f.st.st_mtime);
        if (opts.useColors)
        {
            row.name = colorizeName(f.name, f.st.st_mode);
        }
        else
        {
            row.name = f.name;
        }

        max_links = max(max_links, row.links.length());
        max_owner = max(max_owner, row.owner.length());
        max_group = max(max_group, row.group.length());
        max_size = max(max_size, row.size.length());

        total_blocks += f.st.st_blocks;

        rows.push_back(move(row));
    }

    if (!rows.empty())
    {
        cout << "Total " << (opts.h ? getHumanReadableSize(total_blocks * 512) : to_string(total_blocks / 2)) << "\n";
    }

    for (const auto &r : rows)
    {
        cout << r.perms << " "
             << setw(max_links) << right << r.links << " "
             << setw(max_owner) << left << r.owner << " "
             << setw(max_group) << left << r.group << " "
             << setw(max_size) << right << r.size << " "
             << r.timeStr << " "
             << r.name << "\n";
    }
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");
    Options opts = parseArguments(argc, argv);
    opts.useColors = isatty(STDOUT_FILENO);
    vector<FileInfo> files = readDirectory(opts.dirPath);
    if (files.empty())
        return 0;

    sort(files.begin(), files.end(), [](const FileInfo &a, const FileInfo &b)
         { return a.name < b.name; });

    if (opts.r)
    {
        reverse(files.begin(), files.end());
    }

    if (opts.l)
    {
        printLong(files, opts);
    }
    else
    {
        printShort(files, opts);
    }

    return 0;
}