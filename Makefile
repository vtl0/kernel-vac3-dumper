CC ?= gcc
OPTS ?= -O2 -march=native
LDFLAGS ?= -Wl,-z,relro,-z,now
CFLAGS ?= ${OPTS} -fstack-protector-all -D_FORTIFY_SOURCE=3
BUILD_DIR ?= ./build
SRC_DIR ?= ./src
OBJ_DIR = ${BUILD_DIR}/objs
_OBJ_DIR = $(OBJ_DIR:./%=%)
BIN_DIR = ${BUILD_DIR}/bin
VACD_BIN ?= vacd
KVACD_BIN ?= kvacd
VACD_LIBS ?= -lz

VACD_DIR ?= usermode
KVACD_DIR ?= kernel
VACD_SRC_FILES = main.c controller.c logging.c
KVACD_SRC_FILES = entry.c hooks.c memory.c

_VACD_OBJ_FILES = $(subst /,.,$(VACD_SRC_FILES:.c=.o))
VACD_OBJ_FILES = $(addprefix $(OBJ_DIR)/${VACD_DIR}/,${_VACD_OBJ_FILES})
KVACD_OBJ_FILES = $(subst /,.,$(KVACD_SRC_FILES:.c=.o))

obj-m := ${KVACD_BIN}.o
${KVACD_BIN}-objs = ${KVACD_OBJ_FILES}

.PHONY: all
all: vacd kvacd

${BUILD_DIR}:
	mkdir ${BUILD_DIR}

${BIN_DIR}: ${BUILD_DIR}
	mkdir ${BIN_DIR}

${OBJ_DIR}: ${BUILD_DIR}
	mkdir ${OBJ_DIR}
	mkdir ${OBJ_DIR}/${VACD_DIR}
	mkdir ${OBJ_DIR}/${KVACD_DIR}

${BIN_DIR}/${VACD_BIN}: ${BIN_DIR} ${VACD_OBJ_FILES}
	@echo "Linking ${VACD_BIN}..."
	$(CC) ${VACD_LIBS} ${CFLAGS} ${LDFLAGS} ${VACD_OBJ_FILES} -o ${BIN_DIR}/${VACD_BIN}
	@echo "${VACD_BIN} successfully built to ${BIN_DIR}"

${BIN_DIR}/${KVACD_BIN}.ko: ${OBJ_DIR} ${BIN_DIR}
	@echo "Building ${KVACD_BIN}..."
	$(MAKE) -C /lib/modules/$(shell uname -r)/build KCFLAGS="${CFLAGS}" M="$(shell pwd)/${SRC_DIR}/${KVACD_DIR}" modules
	mv "$(shell pwd)/${SRC_DIR}/${KVACD_DIR}/${KVACD_BIN}.ko" "${BIN_DIR}/"
	@echo "${KVACD_BIN} successfully built to ${BIN_DIR}"

${VACD_OBJ_FILES}: ${OBJ_DIR}
	$(CC) ${CFLAGS} ${LDFLAGS} -c ${SRC_DIR}/${VACD_DIR}/$(subst .,/,$(subst ${_OBJ_DIR}/${VACD_DIR}/,,$(@:.o=))).c -o $@

.PHONY: vacd
vacd: ${BIN_DIR}/${VACD_BIN}

.PHONY: kvacd
kvacd: ${BIN_DIR}/${KVACD_BIN}.ko

.PHONY: clean
clean:
	rm -rf ${BUILD_DIR}
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M="$(shell pwd)/${SRC_DIR}/${KVACD_DIR}" clean
