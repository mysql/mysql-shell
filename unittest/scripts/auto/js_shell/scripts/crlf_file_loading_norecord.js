//@<> Loads Text File With CRLF Line Endings
crlf_file = os.path.join(__test_data_path, "js", "crlf.txt")
data = os.loadTextFile(crlf_file)
EXPECT_EQ("This is a text file with CRLF\r\nline endings.\r\n", data)
