dofile("../common.lua")

RequireDefaultlibs()
--RequireRuntime()


SOLUTION"luasocket"
	INCLUDES	"lua51"
	defines		{"NDEBUG"}
	
	WINDOWS()
	LINUX()

	PROJECT()
		
		INCLUDES	"lua51"
		
		configuration	("windows")
		configuration	("linux")
			links"iberty"
			links_static"libunwind"
			links"pthread"
						