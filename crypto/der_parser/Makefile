PREFIX_DIR := gen
KEY_ID := rsa2048

$(shell mkdir -p $(PREFIX_DIR))


KEYGEN := $(PREFIX_DIR)/rsa2048_private_key_pkcs1.der  \
			$(PREFIX_DIR)/rsa4096_private_key_pkcs1.der

all: $(KEYGEN)
	gcc -o "der_parser" "main.c" -ggdb -g3 -fsanitize=address -Wall

	@echo ==================================================
	./der_parser $(PREFIX_DIR)/rsa2048_private_key_pkcs1.der
	@echo ==================================================
	./der_parser $(PREFIX_DIR)/rsa4096_private_key_pkcs1.der

$(KEYGEN):
	# traditional: use PKCS1 instead of PKCS8
	openssl genrsa \
		-traditional \
		-out $(PREFIX_DIR)/rsa2048_private_key_pkcs1.pem 2048

	openssl genrsa \
		-traditional \
		-out $(PREFIX_DIR)/rsa4096_private_key_pkcs1.pem 4096

	openssl rsa \
		-traditional \
		-in $(PREFIX_DIR)/rsa2048_private_key_pkcs1.pem \
		-outform DER \
		-out $(PREFIX_DIR)/rsa2048_private_key_pkcs1.der

	openssl rsa \
		-traditional \
		-in $(PREFIX_DIR)/rsa4096_private_key_pkcs1.pem \
		-outform DER \
		-out $(PREFIX_DIR)/rsa4096_private_key_pkcs1.der
