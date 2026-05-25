PREFIX_DIR := gen
KEY_ID := rsa4096

$(shell mkdir -p $(PREFIX_DIR))


KEYS := $(PREFIX_DIR)/$(KEY_ID)_private_key.der

.PHONY: all
all: $(KEYS)
	./runner.sh -i gen/rsa4096_private_key.der -o gen/rsa4096_cert_java.der
	openssl x509 -in  gen/rsa4096_cert_java.der -inform der -text -noout


$(KEYS):
	openssl genpkey -algorithm RSA \
		-pkeyopt rsa_keygen_bits:4096 \
		-out $(PREFIX_DIR)/$(KEY_ID)_private_key.pem -quiet
	openssl pkcs8 -topk8 -nocrypt -in $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-outform der -out $(PREFIX_DIR)/$(KEY_ID)_private_key.der

