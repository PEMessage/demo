

PREFIX_DIR := .
KEY_ID := ecc_secp384r1


ALL_KEY := \
		   $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		   $(PREFIX_DIR)/$(KEY_ID)_public_key.pem \
		   $(PREFIX_DIR)/$(KEY_ID)_public_key.der
all:
	mkdir -p $(PREFIX_DIR)
	# 生成secp384r1椭圆曲线私钥
	openssl ecparam -name secp384r1 \
		-genkey \
		-out $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-noout
	# 从私钥中导出公钥
	openssl ec -in $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-pubout -out $(PREFIX_DIR)/$(KEY_ID)_public_key.pem
	# 生成der格式公钥
	openssl pkey -in $(PREFIX_DIR)/$(KEY_ID)_public_key.pem  -pubin \
		-outform der -out $(PREFIX_DIR)/$(KEY_ID)_public_key.der
	# 生成数据
	echo "12345678" > $(PREFIX_DIR)/data.bin
	# 签名
	openssl dgst -sha256 -sign $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-out $(PREFIX_DIR)/data.ec.sig1 $(PREFIX_DIR)/data.bin
	# 签名方式2.1 生成哈希
	openssl dgst -sha256 -binary -out $(PREFIX_DIR)/data.bin.sha256 $(PREFIX_DIR)/data.bin
	# 签名方式2.2 签名
	openssl pkeyutl -sign -in $(PREFIX_DIR)/data.bin.sha256 \
		-inkey $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-out $(PREFIX_DIR)/data.ec.sig2
	# 校验1
	openssl dgst -sha256 -verify $(PREFIX_DIR)/$(KEY_ID)_public_key.pem \
		-signature $(PREFIX_DIR)/data.ec.sig1 $(PREFIX_DIR)/data.bin
	openssl dgst -sha256 -verify $(PREFIX_DIR)/$(KEY_ID)_public_key.pem \
		-signature $(PREFIX_DIR)/data.ec.sig2 $(PREFIX_DIR)/data.bin
	# 校验2
	openssl pkeyutl -verify -in $(PREFIX_DIR)/data.bin.sha256 -sigfile $(PREFIX_DIR)/data.ec.sig1 \
		-inkey $(PREFIX_DIR)/$(KEY_ID)_public_key.pem -pubin
	openssl pkeyutl -verify -in $(PREFIX_DIR)/data.bin.sha256 -sigfile $(PREFIX_DIR)/data.ec.sig2 \
		-inkey $(PREFIX_DIR)/$(KEY_ID)_public_key.pem -pubin
