dofile("../common.lua")

-- apt-get install libunwind7-dev binutils-dev


SOLUTION"segfault"
	INCLUDES	"lua51"
	defines		{"NDEBUG"}
	removedefines	"_GNU_SOURCE"
	WINDOWS()
	LINUX()

	PROJECT()
		
		INCLUDES	"lua51"
		
		configuration	("windows")
		configuration	("linux")
			links"iberty"
			links_static"unwind"
			links"pthread"
						