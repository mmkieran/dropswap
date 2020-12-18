#include "win.h"

#include <windows.h>
#include <shobjidl.h> 
#include <stdio.h>

char* fileOpenUI() {
   HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
   char* path = new char[500];  //to return the final path
   sprintf(path, " ");

   //Create a filter for different file types
   COMDLG_FILTERSPEC rgSpec[] =
   {
       { L"DS Replay Files", L"*.rep" },
       { L"All Files", L"*.*" },
   };

   if (SUCCEEDED(hr))
   {
      IFileOpenDialog* pFileOpen;

      // Create the FileOpenDialog object.
      hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

      if (SUCCEEDED(hr))
      {
         // Set the file types to display only a particular type
         hr = pFileOpen->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);
         if (SUCCEEDED(hr))
         {
            hr = pFileOpen->SetDefaultExtension(L"rep");
            if (SUCCEEDED(hr))
            {
               // Show the Open dialog box.
               hr = pFileOpen->Show(NULL);

               // Get the file name from the dialog box.
               if (SUCCEEDED(hr))
               {
                  IShellItem* pItem;
                  hr = pFileOpen->GetResult(&pItem);
                  if (SUCCEEDED(hr))
                  {
                     PWSTR pszFilePath;
                     hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                     // Display the file name to the user.
                     if (SUCCEEDED(hr))
                     {
                        MessageBoxW(NULL, pszFilePath, L"File Path", MB_OK);
                        wcstombs(path, pszFilePath, 500);
                        CoTaskMemFree(pszFilePath);
                     }
                     pItem->Release();
                  }
               }
            }
         }
      }
      pFileOpen->Release();
      CoUninitialize();
   }
   return path;
}