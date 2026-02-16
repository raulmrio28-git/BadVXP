"C:\Program Files (x86)\MRE SDK V3.0.00\tools\DllPackage.exe" "I:\BadVXP\BadVXP_MRE2.0\BadVXP.vcproj"
if %errorlevel% == 0 (
 echo postbuild OK.
  copy BadVXP.vpp ..\..\..\MoDIS_VC9\WIN32FS\DRIVE_E\BadVXP.vpp /y
exit 0
)else (
echo postbuild error
  echo error code: %errorlevel%
  exit 1
)

