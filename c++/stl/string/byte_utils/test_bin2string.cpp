#include <gtest/gtest.h>
#include "bin2string.cpp" // Include the file with the bin2string function

TEST(Bin2StringTests, EmptyString) {
    EXPECT_EQ(bin2string(""), "");
}

TEST(Bin2StringTests, SingleCharacter) {
    EXPECT_EQ(bin2string("A"), "0x41");
}

TEST(Bin2StringTests, MultipleCharactersLessThan8) {
    EXPECT_EQ(bin2string("Hello"), "0x48, 0x65, 0x6c, 0x6c, 0x6f");
}

TEST(Bin2StringTests, Exactly8Characters) {
    EXPECT_EQ(bin2string("ABCDEFGH"), "0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48");
}

TEST(Bin2StringTests, MoreThan8Characters) {
    EXPECT_EQ(bin2string("HelloWorld"), "0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x57, 0x6f, 0x72, \n0x6c, 0x64");
}

TEST(Bin2StringTests, SpecialCharacters) {
    EXPECT_EQ(bin2string("!@#$%^&*()"), "0x21, 0x40, 0x23, 0x24, 0x25, 0x5e, 0x26, 0x2a, \n0x28, 0x29");
}

TEST(Bin2StringTests, NonPrintableCharacters) {
    EXPECT_EQ(bin2string(std::string("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09",10)), "0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07\n0x08, 0x09");
}


TEST(Bin2StringTests, MixedContent) {
    EXPECT_EQ(bin2string("ABCD1234!@#$"), "0x41, 0x42, 0x43, 0x44, 0x31, 0x32, 0x33, 0x34, \n0x21, 0x40, 0x23, 0x24");
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

