

run:all test/ecc/cert.der test/rsa/cert.der
	./build/print_der test/ecc/cert.der
	./build/print_der test/rsa/cert.der

all:
	mkdir -p build && \
	cd build && \
	cmake \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DCMAKE_BUILD_TYPE=Debug \
		.. && \
	make

test/ecc/cert.der:
	mkdir -p test/ecc
	openssl ecparam -name prime256v1 -genkey -noout -out test/ecc/private.key
	openssl req -x509 -new -key test/ecc/private.key -sha256 -days 365 -out test/ecc/cert.der -outform DER -subj "/CN=Example/C=US/O=MyOrg"
test/rsa/cert.der:
	mkdir -p test/rsa
	openssl genrsa -out test/rsa/private.key
	openssl req -x509 -new -key test/rsa/private.key -sha256 -days 365 -out test/rsa/cert.der -outform DER -subj "/CN=Example/C=US/O=MyOrg"


