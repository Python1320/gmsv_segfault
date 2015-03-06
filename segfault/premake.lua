dofile("../common.lua")

-- apt-get install libunwind7-dev binutils-dev


SOLUTION"segfault"
	INCLUDES	"lua51"
	INCLUDES        "sigscanning"
	defines		{"NO_SOURCE_SDK"}
	removedefines	"_GNU_SOURCE"
	WINDOWS()
	LINUX()

	PROJECT()
		
		INCLUDES	"lua51"
		INCLUDES    "sigscanning"
		
		configuration	("windows")
		configuration	("linux")
			links"iberty"
			links_static"unwind"
			links_static"lzma"
			links"pthread"
			buildoptions 		{ "-fpermissive" }
						