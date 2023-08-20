PROJECT_GENERATOR_VERSION = 3 --v3 has 64 bit support

newoption({
	trigger = "gmcommon",
	description = "Sets the path to the garrysmod_common (https://github.com/danielga/garrysmod_common) directory",
	value = "../garrysmod_common"
})

-- newoption({
-- 	trigger = "shared",
-- 	description = "Sets the path to the shared gspeak code (https://github.com/Thendon/GspeakSharedCode) directory",
-- 	value = "../source/shared"
-- })

local gmcommon = assert(_OPTIONS.gmcommon or os.getenv("GARRYSMOD_COMMON"),
	"you didn't provide a path to your garrysmod_common (https://github.com/danielga/garrysmod_common) directory")
include(gmcommon)

-- local shared = assert(_OPTIONS.shared or os.getenv("GSPEAK_SHARED"),
-- 	"you didn't provide a path to your shared gspeak code (https://github.com/Thendon/GspeakSharedCode) directory")
-- include(shared)

CreateWorkspace({name = "tslib", abi_compatible = false, path = "projects/" .. os.target() .. "/" .. _ACTION})
	-- CreateProject({serverside = true, source_path = "source", manual_files = false})
	-- 	IncludeLuaShared()
	-- 	IncludeScanning()
	-- 	IncludeDetouring()
	-- 	IncludeSDKCommon()
	-- 	IncludeSDKTier0()
	-- 	IncludeSDKTier1()

	CreateProject({serverside = false, source_path = "source", manual_files = false})
		IncludeLuaShared()
		IncludeScanning()
		IncludeDetouring()
		IncludeSDKCommon()
		IncludeSDKTier0()
		IncludeSDKTier1()

		filter("system:windows")
			files({"source/win32/*.cpp", "source/win32/*.hpp"})

		filter("system:linux or macosx")
			files({"source/posix/*.cpp", "source/posix/*.hpp"})
