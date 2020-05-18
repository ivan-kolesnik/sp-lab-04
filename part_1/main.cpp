#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <atlstr.h>
#include <string>

using namespace std;

CString CreateArchivatorProcess(string run_mode, string archive_name, string files) {

    CString str_result = L"";   // pipe output

    HANDLE h_child_stdout_rd;   // Read-side
    HANDLE h_child_stdout_wr;   // Write-side

    //create pipe
    SECURITY_ATTRIBUTES security_attr = { sizeof(SECURITY_ATTRIBUTES) };
    security_attr.bInheritHandle = TRUE;
    security_attr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&h_child_stdout_rd, &h_child_stdout_wr, &security_attr, 0))
    {
        cout << "Cannot create pipe." << endl;
    }

    STARTUPINFO startup_info;
    ZeroMemory(&startup_info, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);

    PROCESS_INFORMATION process_info;
    ZeroMemory(&process_info, sizeof(process_info));

    startup_info.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    startup_info.hStdOutput = h_child_stdout_wr;
    startup_info.hStdError = h_child_stdout_wr;
    startup_info.wShowWindow = SW_HIDE;

    string comand_l_args = "7z " + run_mode + " " + archive_name;
    if (run_mode == "a") {
        comand_l_args += " " + files;
    }
    else
    {
        comand_l_args += " -o" + files;
    }

    //"7z" +mode -tzip archive1.zip listfile.txt"
    LPTSTR szCmdline = _wcsdup(wstring(comand_l_args.begin(), comand_l_args.end()).c_str());
    if (CreateProcess(TEXT("7z.exe"), szCmdline,
        NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &process_info))
    {
        Sleep(1000);
        TerminateProcess(process_info.hProcess, NO_ERROR);
    }
    else
    {
        cout << "Cannot create process.\n" << endl;
    }

    WaitForSingleObject(process_info.hProcess, INFINITE);
    TerminateProcess(process_info.hProcess, 0);

    if (!CloseHandle(h_child_stdout_wr))
    {
        cout << "Cannot close handle." << endl;
    }

    // Read output from the child process.
    for (;;)
    {
        DWORD dw_read;
        CHAR ch_buf[4096];

        // Read from pipe that is the standard output for child process.
        bool done = !ReadFile(h_child_stdout_rd, ch_buf, 4096, &dw_read, NULL) || dw_read == 0;
        if (done)
        {
            break;
        }

        str_result += CString(ch_buf, dw_read);
    }

    CloseHandle(h_child_stdout_rd);
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);

    return str_result;
}

int main()
{
    enum program_mode
    {
        ARCHIVE,
        UNPACK,
        EXIT
    };

    int mode;
    bool is_exit = false;

    string arch_path;
    string file;
    CString run_process_output = L"";

    do
    {
        cout << "Main menu. Choose operation:" << endl
            << "0 - archive a file" << endl
            << "1 - unpack archive" << endl
            << "2 - exit" << endl;
        cin >> mode;

        switch (mode)
        {
            case ARCHIVE:
            {
                cout << "Specify full path to creating archive (name included): ";
                getline(cin, arch_path);
                getline(cin, arch_path);

                cout << "Enter required file (full path, name included): ";
                getline(cin, file);

                cout << "Archiving..." << endl;
                run_process_output = CreateArchivatorProcess("a", arch_path, file);

                break;
            }
            case UNPACK:
            {
                cout << "Specify full path to archive (name included): ";
                getline(cin, arch_path);
                getline(cin, arch_path);

                cout << "Enter destination folder (full path, name included): ";
                getline(cin, file); // 'file' was reused because folder actually is a file

                cout << "Unpacking...\n";
                run_process_output = CreateArchivatorProcess("x", arch_path, file);

                break;
            }
            case EXIT:
            {
                is_exit = true;
                break;
            }
            default:
            {
                break;
            }
        }

        if (run_process_output.Find(_tcsdup(L"Warning"), 0) != -1)
        {
            cout << "Some warning was occured." << endl;
        }
        else if (run_process_output.Find(_tcsdup(L"Error"), 0) != -1)
        {
            cout << "Some warning was occured." << endl;
        }
        else if (run_process_output != L"")
        {
            cout << "Everything went OK." << endl;
        }
    }
    while (!is_exit);

    return 0;
}
