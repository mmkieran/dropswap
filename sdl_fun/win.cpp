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
                        //MessageBoxW(NULL, pszFilePath, L"File Path", MB_OK);
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

char* fileSaveUI() {
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
      IFileSaveDialog* pSaveDialog;

      // Create the FileOpenDialog object.
      hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&pSaveDialog));

      if (SUCCEEDED(hr)) {
         // Set the file types to display only a particular type
         hr = pSaveDialog->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);
         if (SUCCEEDED(hr))
         {
            hr = pSaveDialog->SetDefaultExtension(L"rep");
            if (SUCCEEDED(hr))
            {
               hr = pSaveDialog->Show(NULL);
               if (SUCCEEDED(hr))
               {
                  // Grab the shell item that user specified in the save dialog
                  IShellItem* pSaveResult = NULL;
                  hr = pSaveDialog->GetResult(&pSaveResult);
                  if (SUCCEEDED(hr)) {
                     // Extract the full system path from the shell item
                     PWSTR pszPathName = NULL;
                     hr = pSaveResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPathName);

                     if (SUCCEEDED(hr)) {
                        wcstombs(path, pszPathName, 500);
                     }
                  }
               }
            }
         }
      }

      pSaveDialog->Release();
      CoUninitialize();
   }
   return path;
}

void fileOpenDefaultProgram(wchar_t const* path) {
   ShellExecute(0, 0, path, 0, 0, SW_SHOW);
}