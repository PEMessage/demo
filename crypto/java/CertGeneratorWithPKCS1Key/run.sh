#!/bin/sh
if [ ! -f bcprov-jdk15on-1.70.jar ] ; then
    wget https://repo1.maven.org/maven2/org/bouncycastle/bcprov-jdk15on/1.70/bcprov-jdk15on-1.70.jar
fi
if [ ! -f bcpkix-jdk15on-1.70.jar ] ; then
    wget https://repo1.maven.org/maven2/org/bouncycastle/bcpkix-jdk15on/1.70/bcpkix-jdk15on-1.70.jar
fi


javac -cp bcprov-jdk15on-1.70.jar:bcpkix-jdk15on-1.70.jar CertGeneratorWithPKCS1Key.java -d target/
java  -cp target/:bcprov-jdk15on-1.70.jar:bcpkix-jdk15on-1.70.jar CertGeneratorWithPKCS1Key $1
