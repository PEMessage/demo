

all:
	mkdir -p build && \
	cd build && \
	cmake \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DCMAKE_BUILD_TYPE=Debug \
		.. && \
	make

test/cert.der:
	mkdir -p test
	openssl ecparam -name prime256v1 -genkey -noout -out test/private.key
	openssl req -x509 -new -key test/private.key -sha256 -days 365 -out test/cert.der -outform DER -subj "/CN=Example/C=US/O=MyOrg"

