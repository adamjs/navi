@echo This will clean up all intermediate/user-specific files for NaviLibrary. 
@echo ** You must close Visual C++ before running this file! **
@pause

@echo ========== Cleaning up... ==========
@del Navi.ncb
@del Navi.suo /AH
@del Navi\*.user
@rmdir Navi\Objects\Release\ /S /Q
@rmdir Navi\Objects\Debug\ /S /Q
@del Navi\Lib\*.lib
@del NaviDemo\*.user
@rmdir NaviDemo\Objects\Release\ /S /Q
@rmdir NaviDemo\Objects\Debug\ /S /Q
@rmdir NaviDemo\Bin\release\ /S /Q
@rmdir NaviDemo\Bin\debug\ /S /Q
@echo ============== Done! ===============
