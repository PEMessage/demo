

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
	openssl ecparam -name prime256v1 -genkey -noout -out test/ecc/private.key.pem
	openssl ec -in test/ecc/private.key.pem -out test/ecc/private.key.der -outform der
	openssl ec -in test/ecc/private.key.pem -out test/ecc/public.key.der  -outform der -pubout
	openssl req -x509 -new -key test/ecc/private.key.pem -sha256 -days 365 -out test/ecc/cert.der -outform DER -subj "/CN=Example/C=US/O=MyOrg"
test/rsa/cert.der:
	mkdir -p test/rsa
	openssl genrsa -out test/rsa/private.key.pem
	openssl rsa -in test/rsa/private.key.pem  -out test/rsa/private.key.der -outform der
	openssl rsa -in test/rsa/private.key.pem  -out test/rsa/public.key.der  -outform der -pubout
	openssl req -x509 -new -key test/rsa/private.key.pem -sha256 -days 365 -out test/rsa/cert.der -outform DER -subj "/CN=Example/C=US/O=MyOrg"


