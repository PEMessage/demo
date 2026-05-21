

PREFIX_DIR := gen
KEY_ID := ecc_secp384r1

$(shell mkdir -p $(PREFIX_DIR))

SHELL = ./color_shell.sh

# =======================================
KEYGEN := $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		  $(PREFIX_DIR)/$(KEY_ID)_private_key_sec1.pem
# =======================================
KEYS :=  $(PREFIX_DIR)/$(KEY_ID)_private_key.der \
		 $(PREFIX_DIR)/$(KEY_ID)_private_key_sec1.der \
		 $(PREFIX_DIR)/$(KEY_ID)_public_key.pem \
		 $(PREFIX_DIR)/$(KEY_ID)_public_key.der

# =======================================
.PHONY: verify run keygen keys
run: verify
keygen: $(KEYGEN)
keys: $(KEYS)
# =======================================

$(KEYGEN):
	mkdir -p $(PREFIX_DIR)
	# ======================================
	# 生成secp384r1椭圆曲线私钥(SEC1 ECPrivateKey)
	# ======================================
	openssl ecparam -name secp384r1 \
		-genkey \
		-out $(PREFIX_DIR)/$(KEY_ID)_private_key_sec1.pem \
		-noout
	openssl asn1parse -dump -in $(PREFIX_DIR)/$(KEY_ID)_private_key_sec1.pem

	# ======================================
	# 生成secp384r1椭圆曲线私钥(PKCS8)
	# ======================================
	# PrivateKeyInfo ::= SEQUENCE {
	#   version         INTEGER { v1(0) },
	#   privateKeyAlgorithm   PrivateKeyAlgorithmIdentifier,
	#   privateKey      OCTET STRING,   -- 内层包含 ECPrivateKey
	#   attributes      [0] IMPLICIT Attributes OPTIONAL
	# }
	openssl genpkey -algorithm EC \
		-pkeyopt ec_paramgen_curve:secp384r1 \
		-out $(PREFIX_DIR)/$(KEY_ID)_private_key.pem
	openssl asn1parse -dump -in $(PREFIX_DIR)/$(KEY_ID)_private_key.pem

$(KEYS): $(KEYGEN)
	mkdir -p $(PREFIX_DIR)

	# ======================================
	# (1) pem -> der格式私钥 (SEC1 ECPrivateKey 会剥离pkcs8外壳)
	# ======================================
	openssl pkey -in $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-outform der -out $(PREFIX_DIR)/$(KEY_ID)_private_key_sec1.der
	openssl asn1parse -dump -in $(PREFIX_DIR)/$(KEY_ID)_private_key_sec1.der -inform der

	# another not uniform way
	openssl pkcs8 -nocrypt -in $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-outform der -out $(PREFIX_DIR)/$(KEY_ID)_private_key_sec1.der
	openssl asn1parse -dump -in $(PREFIX_DIR)/$(KEY_ID)_private_key_sec1.der -inform der

	# ======================================
	# (2) pem -> der格式私钥 (保留pkcs8 外壳)
	# --
	#  -topk8: 默认输出仍然是 SEC1 ECPrivateKey
	#  -nocrypt: 必须加, 否则解析失败
	# ======================================
	openssl pkcs8 -topk8 -nocrypt -in $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-outform der -out $(PREFIX_DIR)/$(KEY_ID)_private_key.der
	openssl asn1parse -dump -in $(PREFIX_DIR)/$(KEY_ID)_private_key.der -inform der

	# ======================================
	# pem 私钥 -> pem 公钥
	# ======================================

	openssl pkey -in $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
             -pubout -out $(PREFIX_DIR)/$(KEY_ID)_public_key.pem
	openssl asn1parse -dump -in $(PREFIX_DIR)/$(KEY_ID)_public_key.pem

	# another not uniform way
	openssl ec -in $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-pubout -out $(PREFIX_DIR)/$(KEY_ID)_public_key.pem
	openssl asn1parse -dump -in $(PREFIX_DIR)/$(KEY_ID)_public_key.pem

	# ======================================
	# pem 公钥 -> pem 公钥
	# ======================================
	openssl pkey -in $(PREFIX_DIR)/$(KEY_ID)_public_key.pem  -pubin \
		-outform der -out $(PREFIX_DIR)/$(KEY_ID)_public_key.der
	openssl asn1parse -dump -in $(PREFIX_DIR)/$(KEY_ID)_public_key.der -inform der

ALL_KEYS := $(KEYGEN) $(KEYS)

# =======================================
DATAS := \
		 $(PREFIX_DIR)/data.bin
$(DATAS):
	mkdir -p $(PREFIX_DIR)
	# 生成Default数据
	echo "12345678" > $(PREFIX_DIR)/data.bin

# =======================================
SIGNATURES := \
			  $(PREFIX_DIR)/data.ec.sig1 \
			  $(PREFIX_DIR)/data.ec.sig2

$(PREFIX_DIR)/data.ec.sig1: $(DATAS) $(KEYS)
	# 签名
	openssl dgst -sha256 -sign $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-out $(PREFIX_DIR)/data.ec.sig1 $(PREFIX_DIR)/data.bin
	openssl asn1parse -dump -in $@ -inform der

$(PREFIX_DIR)/data.ec.sig2: $(DATAS) $(KEYS)
	# 签名方式2.1 生成哈希
	openssl dgst -sha256 -binary -out $(PREFIX_DIR)/data.bin.sha256 $(PREFIX_DIR)/data.bin
	# 签名方式2.2 签名
	openssl pkeyutl -sign -in $(PREFIX_DIR)/data.bin.sha256 \
		-inkey $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-out $(PREFIX_DIR)/data.ec.sig2
	openssl asn1parse -dump -in $@ -inform der

# =======================================
verify: $(SIGNATURES) $(DATAS) $(KEYS)
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
