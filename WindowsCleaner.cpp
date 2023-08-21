#include <iostream>
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <string>
#include <vector>
using namespace std;

void cleanDirectory(const wstring& directory, ULONGLONG& totalSize) {
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW((directory + L"\\*").c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            const wstring fileName = findData.cFileName;

            if (fileName != L"." && fileName != L"..") {
                const wstring fullPath = directory + L"\\" + fileName;

                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    cleanDirectory(fullPath, totalSize);
                    RemoveDirectoryW(fullPath.c_str());
                }
                else {
                    ULONGLONG fileSize = static_cast<ULONGLONG>(findData.nFileSizeLow) +
                        (static_cast<ULONGLONG>(findData.nFileSizeHigh) << 32);
                    totalSize += fileSize;
                    DeleteFileW(fullPath.c_str());
                }
            }
        } while (FindNextFileW(hFind, &findData));

        FindClose(hFind);
    }
}

ULONGLONG calculateDirectorySize(const wstring& directory) {
    ULONGLONG totalSize = 0;

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW((directory + L"\\*").c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            const wstring fileName = findData.cFileName;

            if (fileName != L"." && fileName != L"..") {
                const wstring fullPath = directory + L"\\" + fileName;

                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    ULONGLONG fileSize = static_cast<ULONGLONG>(findData.nFileSizeLow) +
                        (static_cast<ULONGLONG>(findData.nFileSizeHigh) << 32);
                    totalSize += fileSize;
                }
                else {
                    totalSize += calculateDirectorySize(fullPath);  // 递归计算子目录大小
                }
            }
        } while (FindNextFileW(hFind, &findData));

        FindClose(hFind);
    }

    return totalSize;
}

void cleanCache(const wstring& cachePath, const wchar_t* cacheName) {
    ULONGLONG totalSize = calculateDirectorySize(cachePath);
    cleanDirectory(cachePath, totalSize);
    cout << "已清理" << cacheName << "缓存大小: " << totalSize / (1024 * 1024) << " MB\n";
}

void emptyRecycleBin() {
    SHEmptyRecycleBin(NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
    ULONGLONG recycleBinSize = calculateDirectorySize(L"C:\\$Recycle.Bin");
    cout << "已清理文件大小: " << recycleBinSize / (1024 * 1024) << " MB\n";
}

vector<wstring> findLargeFiles(const wstring& directory, ULONGLONG thresholdSize) {
    vector<wstring> largeFiles;

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW((directory + L"\\*").c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            const wstring fileName = findData.cFileName;

            if (fileName != L"." && fileName != L"..") {
                const wstring fullPath = directory + L"\\" + fileName;

                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    ULONGLONG fileSize = static_cast<ULONGLONG>(findData.nFileSizeLow) +
                        (static_cast<ULONGLONG>(findData.nFileSizeHigh) << 32);

                    if (fileSize >= thresholdSize) {
                        largeFiles.push_back(fullPath);
                    }
                }
                else {
                    vector<wstring> subLargeFiles = findLargeFiles(fullPath, thresholdSize);  // 递归查找子目录中的大文件
                    largeFiles.insert(largeFiles.end(), subLargeFiles.begin(), subLargeFiles.end());
                }
            }
        } while (FindNextFileW(hFind, &findData));

        FindClose(hFind);
    }

    return largeFiles;
}

int main() {
    while (true) {
        int choice;
        cout << "Windows Cleaner - 清理垃圾文件和软件缓存\n";
        cout << "以下为可选功能\n";
        cout << "1. 清理系统盘temp文件夹（无风险）\n";
        cout << "2. 删除回收站文件\n";
        cout << "3. 清理QQ缓存\n";
        cout << "4. 清理微信缓存\n";
        cout << "5. 查找磁盘中大文件\n";
        cout << "6. 退出\n";
        cout << "请输入:\n";
        cin >> choice;

        if (choice == 1) {
            wchar_t tempPath[MAX_PATH];
            GetTempPathW(MAX_PATH, tempPath);
            wstring tempDir(tempPath);
            ULONGLONG totalSize = calculateDirectorySize(tempDir);

            cleanDirectory(tempDir, totalSize);
            totalSize /= (1024 * 1024);

            cout << "已清理文件大小: " << totalSize << " MB\n";
        }
        else if (choice == 2) {
            emptyRecycleBin();
            cout << "回收站已清空\n";
        }
        else if (choice == 3) {
            cleanCache(L"C:\\Tencent\\QQ\\QQBrowser\\Cache", L"QQ");
        }
        else if (choice == 4) {
            cleanCache(L"C:\\Tencent\\WeChat\\Cache", L"微信");
        }
        else if (choice == 5) {
            ULONGLONG thresholdSize;
            cout << "请输入要搜索的文件大小阈值（以兆字节为单位）:\n";
            cin >> thresholdSize;

            thresholdSize *= (1024 * 1024); // 将输入的兆字节转换为字节

            wstring drivePath;
            cout << "请输入要搜索的磁盘路径（如C:\\）:\n";
            cin.ignore();  // 忽略之前的换行符
            getline(wcin, drivePath);

            vector<wstring> largeFiles = findLargeFiles(drivePath, thresholdSize);
            if (!largeFiles.empty()) {
                cout << "以下是大于等于指定阈值的文件:\n";
                for (const auto& filePath : largeFiles) {
                    wcout << filePath << endl;
                }
            }
            else {
                cout << "未找到大于等于指定阈值的文件\n";
            }
        }
        else if (choice == 6) {
            break;
        }

        cout << endl;
    }

    return 0;
}
