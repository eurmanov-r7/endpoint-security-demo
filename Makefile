GCC = gcc
GPP = g++
EPS_EXE = eps-example-test
PY_BUILD_DIR = dist
OBJECT_DIR = objects
BUILD_DIR = build
LIBS_DIR = libs
EPS_EXE_PATH = ${PY_BUILD_DIR}/${EPS_EXE}

LD_SHARED_LIBS = -lEndpointSecurity -lbsm
CHECK_EPS_ENTITLE = codesign --entitlements - -d ${PY_BUILD_DIR}/${EPS_EXE} | grep com.apple.developer.endpoint-security.client

run: ${EPS_EXE_PATH}

${LIBS_DIR}:
	mkdir ${LIBS_DIR}

${OBJECT_DIR}:
	mkdir ${OBJECT_DIR}

${BUILD_DIR}:
	mkdir ${BUILD_DIR}

${OBJECT_DIR}/eps_lib.so: ${OBJECT_DIR} eps_lib.cpp eps_lib.h
	${GPP} -fPIC -shared ${LD_SHARED_LIBS} eps_lib.cpp -o ${OBJECT_DIR}/eps_lib.so

${EPS_EXE_PATH}: ${EPS_EXE}.py ${OBJECT_DIR}/eps_lib.so
	pyinstaller --onefile ${EPS_EXE}.py
	codesign -f --entitlement entitlements.plist -s - ${EPS_EXE_PATH}
	${CHECK_EPS_ENTITLE}
	rm ${EPS_EXE}.spec

cpp_scratch: ${LIBS_DIR} ${OBJECT_DIR}
	${GPP} -fPIC -shared ${LIBS_DIR}/custom_struct.c -o ${OBJECT_DIR}/custom_struct.so
	${GPP} -fPIC -shared -I${LIBS_DIR} ${OBJECT_DIR}/custom_struct.so deprecated/cpp_scratch_pad.cpp -o ${OBJECT_DIR}/cpp_scratch.so

clean:
	rm -rf ${LIBS_DIR} ${OBJECT_DIR} ${BUILD_DIR} ${PY_BUILD_DIR}
	rm -f ${OBJECT_DIR}/*.*o
	
