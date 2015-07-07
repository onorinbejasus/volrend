if(WIN32)
    file(INSTALL DESTINATION ${PACKAGE_DIR}/bin
        TYPE FILE
        FILES
            ${BIN_DIR}/volrend.pyd
        )
else
    file(INSTALL DESTINATION ${PACKAGE_DIR}/bin
        TYPE FILE
        FILES
            ${BIN_DIR}/volrend.so
        )
endif()

# Create volrend example package
set(PACKAGE_NAME volrend.demo)
set(PACKAGE_DISPLAY_NAME "Volrend Demo")
set(PACKAGE_DESCRIPTION "Sample data and script for the volrend module")
setup_package()
file(INSTALL DESTINATION ${PACKAGE_DIR}/modules/volrend
    TYPE FILE
    FILES
        ${SOURCE_DIR}/modules/volrend/demo.py
        ${SOURCE_DIR}/modules/volrend/rabbit.tif
    )

if(WIN32)
    file(WRITE ${PACKAGE_DIR}/volrend_Demo.bat 
        ".\\bin\\orun.exe -D %~dp0% volrend/demo.py")
endif()