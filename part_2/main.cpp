#include <windows.h> 
#include <iostream> 
#include <vector>

#define BUF_SIZE 255

using namespace std;

DWORD dw_tls_index;
HANDLE gh_mutex;

void failure_exit(const char * lpsz_message)
{
    fprintf(stderr, "%s\n", lpsz_message);
    ExitProcess(0);
}

vector<int> generate_array(int size) {
    vector<int> row;
    for (size_t i = 0; i < size; i++)
    {
        row.push_back(rand() % 90 + 10);
    }

    return row;
}

// find biggest divider which
// is not the number itself
int biggest_divider(int a) {
    for (int i = a - 1; i > 0; i--)
    {
        if (a & i == 0)
        {
            return i;
        }
    }

    return 1;
}

void print_sum_from_tls()
{
    vector<int>& pointer = *(vector<int>*)TlsGetValue(dw_tls_index);
    int sum = 0;

    if ((pointer[0] == 0) && (GetLastError() != ERROR_SUCCESS))
    {
        failure_exit("TlsGetValue error");
    }
    else
    {
        for (size_t i = 0; i < pointer.size(); i++)
        {
            sum += pointer[i];
        }
    }

    printf("Thread %d is printing sum...It is %d\n", GetCurrentThreadId(), sum);
}

void do_array_calculations(LPVOID lp_param) {
    vector<int>& pointer = *(vector<int>*)lp_param;

    printf("Thread %d is printing array...\n", GetCurrentThreadId());

    LPVOID lpv_data = TlsGetValue(GetCurrentThreadId());
    vector<int> results;

    for (size_t i = 0; i < pointer.size(); i++)
    {
        printf("%d ", pointer[i]);

        int res = biggest_divider(pointer[i]);
        results.push_back(res);
    }

    printf("\n");

    TlsSetValue(dw_tls_index, (vector<int>*) &(results));
    print_sum_from_tls();

    printf("\n");
}

DWORD WINAPI array_calculations_routine(LPVOID lp_param)
{
    HANDLE h_stdout;
    LPVOID lpv_data;

    // Initialize the TLS index for this thread. 
    lpv_data = (LPVOID)LocalAlloc(LPTR, 256);
    if (!TlsSetValue(dw_tls_index, lpv_data))
    {
        failure_exit("TlsSetValue error");
    }
        
    DWORD dw_count = 0;
    DWORD dw_wait_result;

    // Request ownership of mutex.
    dw_wait_result = WaitForSingleObject(
        gh_mutex,    // handle to mutex
        INFINITE
    );

    switch (dw_wait_result)
    {
        // The thread got ownership of the mutex
        case WAIT_OBJECT_0:
        {
            __try
            {
                do_array_calculations(lp_param);
            }
            __finally
            {
                // Release ownership of the mutex object
                if (!ReleaseMutex(gh_mutex))
                {
                    failure_exit("ReleaseMutex error");
                }
            }

            break;
        }
        // The thread got ownership of an abandoned mutex
        case WAIT_ABANDONED:
        {
            return FALSE;
        } 
    }

    lpv_data = TlsGetValue(dw_tls_index);

    if (lpv_data != 0)
    {
        LocalFree((HLOCAL)lpv_data);
    }

    return 0;
}

int main()
{
    // number of threads
    int threads_count;
    cout << "Enter number oh threads: ";
    cin >> threads_count;

    vector<HANDLE> v_h_thread; // threads handle vector
    vector<DWORD> v_id_thread; // threads ID vector

    vector<vector<int>> data;
    vector<vector<int>*> data_pointers;

    // Allocate a TLS index. 
    if ((dw_tls_index = TlsAlloc()) == TLS_OUT_OF_INDEXES)
    {
        failure_exit("TlsAlloc failed");
    }

    for (size_t i = 0; i < threads_count; i++)
    {
        v_id_thread.push_back(NULL);
    }

    gh_mutex = CreateMutex(NULL, FALSE, NULL);

    if (gh_mutex == NULL)
    {
        printf("CreateMutex error: %d\n", GetLastError());
        return 1;
    }

    //creating data sets
    for (size_t i = 0; i < threads_count; i++)
    {
        data.push_back(generate_array(rand() % 5 + 5));
    }

    //getting pointers to data sets
    for (size_t i = 0; i < threads_count; i++)
    {
        data_pointers.push_back((vector<int>*)&(data[i]));
    }

    //creating threads
    for (size_t i = 0; i < threads_count; i++)
    {
       v_h_thread.push_back(NULL);
       v_h_thread[i] = CreateThread(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE)array_calculations_routine,
            data_pointers[i],
            0,
            &v_id_thread[i]
        );

        // Check the return value for success
        if (v_h_thread[i] == NULL)
        {
            failure_exit("CreateThread error\n");
        }
    }

    WaitForMultipleObjects(threads_count, v_h_thread.data(), TRUE, INFINITE);

    for (size_t i = 0; i < threads_count; i++)
    {
        CloseHandle(v_h_thread[i]);
    }

    TlsFree(dw_tls_index);
    CloseHandle(gh_mutex);

    return 0;
}
