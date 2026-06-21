PREFIX_DIR := gen
KEY_ID := rsa2048

$(shell mkdir -p $(PREFIX_DIR))


KEYGEN := $(PREFIX_DIR)/$(KEY_ID)_private_key_pkcs1.der

all: $(KEYGEN)
	gcc -o "der_parser" "main.c" -ggdb -g3 -fsanitize=address -Wall
	./der_parser $(KEYGEN)

$(KEYGEN):
	# traditional: use PKCS1 instead of PKCS8
	openssl genrsa \
		-traditional \
		-out $(PREFIX_DIR)/$(KEY_ID)_private_key_pkcs1.pem 2048

	openssl rsa \
		-traditional \
		-in $(PREFIX_DIR)/$(KEY_ID)_private_key_pkcs1.pem \
		-outform DER \
		-out $(PREFIX_DIR)/$(KEY_ID)_private_key_pkcs1.der
