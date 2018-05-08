solution ('test')
    configurations { 'Release', 'Debug'}
        
    project ('test')
        kind 'ConsoleApp'
		language 'C++'
		
		files {'*.cpp', '*.h', '*.hpp'}
		
		includedirs{'include/'}
		includedirs{'include/stb_image/'}
        
		includedirs{'C:/msys64/mingw64/include/bullet/'}
		
        links{'glew32', 'glfw3', 'opengl32'}
		links{"BulletDynamics", "BulletCollision", "LinearMath"}		

        buildoptions{'-Wwrite-strings'}
	    buildoptions{'-fpermissive'}
        buildoptions{'-Wno-write-strings'}