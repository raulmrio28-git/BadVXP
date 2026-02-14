"C:\Program Files (x86)\MRE SDK V3.0.00\tools\DllPackage.exe" "I:\BadVXP\BadVXP\BadVXP.vcproj"
if %errorlevel% == 0 (
 echo postbuild OK.
  copy BadVXP.dll "C:\Program Files (x86)\MRE SDK V3.0.00\models\Model01_QVGA_MRE3.0\WIN32FS\DRIVE_E\BadVXP.vxp" /y
exit 0
)else (
echo postbuild error
  echo error code: %errorlevel%
  exit 1
)

