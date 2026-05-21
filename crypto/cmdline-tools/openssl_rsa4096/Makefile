

PREFIX_DIR := gen
KEY_ID := rsa4096

$(shell mkdir -p $(PREFIX_DIR))

SHELL = ./color_shell.sh

# =======================================
KEYGEN := $(PREFIX_DIR)/$(KEY_ID)_private_key_pkcs1.pem \
		  $(PREFIX_DIR)/$(KEY_ID)_private_key.pem
# =======================================
KEYS :=  $(PREFIX_DIR)/$(KEY_ID)_private_key_pkcs1.der \
		 $(PREFIX_DIR)/$(KEY_ID)_private_key.der \
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
	# 生成RSA4096私钥 (传统PKCS#1 RSAPrivateKey格式)
	# ======================================
	# RSAPrivateKey ::= SEQUENCE {
	#   version         Version,
	#   modulus         INTEGER,        -- n
	#   publicExponent  INTEGER,        -- e
	#   privateExponent INTEGER,        -- d
	#   prime1          INTEGER,        -- p
	#   prime2          INTEGER,        -- q
	#   exponent1       INTEGER,        -- d mod (p-1)
	#   exponent2       INTEGER,        -- d mod (q-1)
	#   coefficient     INTEGER,        -- (inverse of q) mod p
	#   otherPrimeInfos OtherPrimeInfos OPTIONAL
	# }
	openssl genrsa -out $(PREFIX_DIR)/$(KEY_ID)_private_key_pkcs1.pem 4096
	openssl asn1parse -dump -in $(PREFIX_DIR)/$(KEY_ID)_private_key_pkcs1.pem

	# ======================================
	# 生成RSA4096私钥 (PKCS#8格式)
	# ======================================
	# PrivateKeyInfo ::= SEQUENCE {
	#   version         INTEGER { v1(0) },
	#   privateKeyAlgorithm   PrivateKeyAlgorithmIdentifier,
	#   privateKey      OCTET STRING,   -- 内层包含 RSAPrivateKey
	#   attributes      [0] IMPLICIT Attributes OPTIONAL
	# }
	# # 字符含义：OpenSSL 生成 4096 位 RSA 密钥（寻找两个 2048 位大素数）的进度条
	# . (点)  : 随机生成了一个候选奇数，并通过了小素数除法的初步筛选。
	# + (加号) : 候选数通过了 Miller-Rabin 素性检验的其中一轮测试（每过一轮打一个 +）。
	# * (星号) : 找到符合条件的素数！由于需要 p 和 q 两个素数，该过程会完整跑两轮。
	# -quiet 默认不打印过程
	openssl genpkey -algorithm RSA \
		-pkeyopt rsa_keygen_bits:4096 \
		-out $(PREFIX_DIR)/$(KEY_ID)_private_key.pem -quiet
	openssl asn1parse -dump -in $(PREFIX_DIR)/$(KEY_ID)_private_key.pem

$(KEYS): $(KEYGEN)
	mkdir -p $(PREFIX_DIR)

	# ======================================
	# (1) pem -> der格式私钥 (PKCS#1 RSAPrivateKey, 剥离PKCS8外壳)
	# ======================================
	openssl pkey -in $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-outform der -out $(PREFIX_DIR)/$(KEY_ID)_private_key_pkcs1.der
	openssl asn1parse -dump -in $(PREFIX_DIR)/$(KEY_ID)_private_key_pkcs1.der -inform der

	# another not uniform way
	openssl pkcs8 -nocrypt -in $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-outform der -out $(PREFIX_DIR)/$(KEY_ID)_private_key_pkcs1.der
	openssl asn1parse -dump -in $(PREFIX_DIR)/$(KEY_ID)_private_key_pkcs1.der -inform der

	# ======================================
	# (2) pem -> der格式私钥 (保留PKCS8外壳)
	# --
	#  -topk8: 默认输出仍然是 PKCS#1 RSAPrivateKey
	#  -nocrypt: 必须加, 否则解析失败
	# ======================================
	openssl pkcs8 -topk8 -nocrypt -in $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-outform der -out $(PREFIX_DIR)/$(KEY_ID)_private_key.der
	openssl asn1parse -dump -in $(PREFIX_DIR)/$(KEY_ID)_private_key.der -inform der

	# ======================================
	# pem 私钥 -> pem 公钥
	# ======================================
	# SubjectPublicKeyInfo ::= SEQUENCE {
	#   algorithm       AlgorithmIdentifier,
	#   subjectPublicKey  BIT STRING (RSAPublicKey)
	# }
	# RSAPublicKey ::= SEQUENCE {
	#   modulus         INTEGER,        -- n
	#   publicExponent  INTEGER         -- e
	# }
	openssl pkey -in $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
			 -pubout -out $(PREFIX_DIR)/$(KEY_ID)_public_key.pem
	openssl asn1parse -dump -in $(PREFIX_DIR)/$(KEY_ID)_public_key.pem

	# another not uniform way
	openssl rsa -in $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-pubout -out $(PREFIX_DIR)/$(KEY_ID)_public_key.pem
	openssl asn1parse -dump -in $(PREFIX_DIR)/$(KEY_ID)_public_key.pem

	# ======================================
	# pem 公钥 -> der 公钥
	# ======================================
	openssl pkey -in $(PREFIX_DIR)/$(KEY_ID)_public_key.pem -pubin \
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
			  $(PREFIX_DIR)/data.rsa.sig1 \
			  $(PREFIX_DIR)/data.rsa.sig2 \
			  $(PREFIX_DIR)/data.rsa.sig3

$(PREFIX_DIR)/data.rsa.sig1: $(DATAS) $(KEYS)
	# ======================================
	# 签名方式1: dgst -sign (自动hash+PKCS#1 v1.5 padding)
	# ======================================
	openssl dgst -sha256 -sign $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-out $@ $(PREFIX_DIR)/data.bin
	# RSA签名是原始大整数(512字节), 不是ASN.1结构, 用xxd查看
	xxd -l 64 $@

$(PREFIX_DIR)/data.rsa.sig2: $(DATAS) $(KEYS)
	# ======================================
	# 签名方式2: 先hash, 再pkeyutl -sign (PKCS#1 v1.5 + DigestInfo)
	# ======================================
	# 2.1 对数据做hash
	openssl dgst -sha256 -binary -out $(PREFIX_DIR)/data.bin.sha256 $(PREFIX_DIR)/data.bin
	# 2.2 对hash值签名 (需指定digest算法以便构造DigestInfo)
	openssl pkeyutl -sign -in $(PREFIX_DIR)/data.bin.sha256 \
		-inkey $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-out $@ \
		-pkeyopt digest:sha256
	xxd -l 64 $@

$(PREFIX_DIR)/data.rsa.sig3: $(DATAS) $(KEYS)
	# ======================================
	# 签名方式3: pkeyutl -sign (PSS padding, 更现代更安全)
	# ======================================
	openssl dgst -sha256 -binary -out $(PREFIX_DIR)/data.bin.sha256 $(PREFIX_DIR)/data.bin
	openssl pkeyutl -sign -in $(PREFIX_DIR)/data.bin.sha256 \
		-inkey $(PREFIX_DIR)/$(KEY_ID)_private_key.pem \
		-out $@ \
		-pkeyopt digest:sha256 \
		-pkeyopt rsa_padding_mode:pss \
		-pkeyopt rsa_pss_saltlen:32
	xxd -l 64 $@

# =======================================
verify: $(SIGNATURES) $(DATAS) $(KEYS)
	# ======================================
	# 校验方式1: dgst -verify (对应签名方式1)
	# ======================================
	openssl dgst -sha256 -verify $(PREFIX_DIR)/$(KEY_ID)_public_key.pem \
		-signature $(PREFIX_DIR)/data.rsa.sig1 $(PREFIX_DIR)/data.bin
	# 方式2的签名也可以被dgst -verify验证, 因为都是PKCS#1 v1.5
	openssl dgst -sha256 -verify $(PREFIX_DIR)/$(KEY_ID)_public_key.pem \
		-signature $(PREFIX_DIR)/data.rsa.sig2 $(PREFIX_DIR)/data.bin

	# ======================================
	# 校验方式2: pkeyutl -verify (对应签名方式2)
	# ======================================
	openssl pkeyutl -verify -in $(PREFIX_DIR)/data.bin.sha256 \
		-sigfile $(PREFIX_DIR)/data.rsa.sig2 \
		-inkey $(PREFIX_DIR)/$(KEY_ID)_public_key.pem -pubin \
		-pkeyopt digest:sha256
	# 方式1的签名也可以被pkeyutl -verify验证
	openssl pkeyutl -verify -in $(PREFIX_DIR)/data.bin.sha256 \
		-sigfile $(PREFIX_DIR)/data.rsa.sig1 \
		-inkey $(PREFIX_DIR)/$(KEY_ID)_public_key.pem -pubin \
		-pkeyopt digest:sha256

	# ======================================
	# 校验方式3: pkeyutl -verify (对应签名方式3, PSS padding)
	# ======================================
	openssl pkeyutl -verify -in $(PREFIX_DIR)/data.bin.sha256 \
		-sigfile $(PREFIX_DIR)/data.rsa.sig3 \
		-inkey $(PREFIX_DIR)/$(KEY_ID)_public_key.pem -pubin \
		-pkeyopt digest:sha256 \
		-pkeyopt rsa_padding_mode:pss \
		-pkeyopt rsa_pss_saltlen:32
