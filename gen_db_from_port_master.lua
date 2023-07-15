-- LUA Script for generating database from port-master
-- 18:41 08 June 2023
-- 1. https://github.com/PortsMaster/PortMaster-Releases/releases/latest/
-- 2. https://github.com/PortsMaster/PortMaster-Releases/releases/expanded_assets/2023-06-02_0241

local function fileExists(path)
	local attributes = lfs.attributes(path)
	return attributes and attributes.mode == "file"
end

DB_PATH = "/userdata/roms/bin/es/portmaster.db"
NEW_DB_PATH = "/userdata/roms/bin/es/new-portmaster.db"
-- DB_PATH = "portmaster.db"
-- NEW_DB_PATH = "new-portmaster.db"

local t_local_db = {}
local str_local_db = ""

if fileExists(DB_PATH) then
	for line in io.lines(DB_PATH) do
		local f
		f = csv.parse(line, '|')
		t_local_db[#t_local_db + 1] = f[4]
	end
	str_local_db = table.concat(t_local_db, "|"):lower()
	t_local_db = nil
end

local rc, headers, content = http.request('https://github.com/PortsMaster/PortMaster-Releases/releases/latest/')
if rc ~= 0 then
	print("1. Error: "..http.error(rc), rc)
	return false
end

local url = content:match('src="(https://github.com/PortsMaster/PortMaster%-Releases/releases/expanded_assets/.-)"')
if url == nil then
	print('Invalid response!')
	return false
end
local data_timestamp = url:match('assets/(.-)$')

rc, headers, content = http.request(url)
if rc ~= 0 then
	print("2. Error: "..http.error(rc), rc)
	return false
end

local tsize = {}
local no = 1
for w1,w2 in content:gmatch('class="Truncate%-text text%-bold">(.-)</span>.-class="color%-fg%-muted text%-sm%-left flex%-auto ml%-md%-3">(.-)</span>') do
	if w1:sub(-3) ~= "md5" then
		tsize[w1:lower()] = w2
		no = no + 1
	end
end
content = nil

os.remove('ports.md')
rc, headers = http.request{url = 'https://github.com/PortsMaster/PortMaster-Releases/releases/latest/download/ports.md', output_filename = 'ports.md'}
if rc ~= 0 then
	print("Error: "..http.error(rc), rc)
	return false
end

local fo = io.open("new-portmaster.db.tmp", "w")
if fo == nil then
	print('Error open a file: new-portmaster.db.tmp')
	return false
end

os.remove(DB_PATH)
local fo2 = io.open(DB_PATH, "w")
if fo2 == nil then
	print('Error open a file: '..DB_PATH)
	return false
end

local counter_new = 0
local counter_data = 0
for line in io.lines("ports.md") do
	if #line > 0 then
		local size, mono, tipe
		local title,desc,porter,locat,runtype,genres = line:match('Title.-="(.-) %." Desc="(.-)" porter="(.-)" locat="(.-)" runtype="(.-)" genres="(.-)"')
		tipe = nil
		if runtype ~= nil then
			locat = locat:gsub("%%20","%.")
			locat = locat:gsub("%.%.","%.")
			size = tsize[locat:lower()]
			tipe = "rtr"	-- Ready to Run
		else
			title,desc,porter,locat,mono,genres = line:match('Title.-="(.-) %." Desc="(.-)" porter="(.-)" locat="(.-)" mono="(.-)" genres="(.-)"')
			if mono ~= nil then
				locat = locat:gsub("%%20","%.")
				locat = locat:gsub("%.%.","%.")
				size = tsize[locat:lower()]
				tipe = "mon"	-- Need Mono
			else
				title,desc,porter,locat,genres = line:match('Title.-="(.-) %." Desc="(.-)" porter="(.-)" locat="(.-)" genres="(.-)"')
				if locat ~= nil then
					locat = locat:gsub("%%20","%.")
					locat = locat:gsub("%.%.","%.")
					size = tsize[locat:lower()]
					tipe = "ext"	-- Need extra original game data
				end
			end
		end
		if tipe ~= nil and title ~= nil and desc ~= nil and locat ~= nil and size ~= nil and genres ~= nil then
			fo2:write(string.format("%s|%s|%s|%s|%s|%s\n",tipe,title,desc,locat,size,genres))
			counter_data = counter_data + 1
			if str_local_db:find(locat:lower(), 1, true) == nil then
				fo:write(string.format("%s|%s|%s|%s|%s|%s\n",tipe,title,desc,locat,size,genres))
				counter_new = counter_new + 1
			end
		else
			print("err", tipe,title,desc,locat,size,genres)
		end
	end
end
fo2:close()
fo:close()

print("Latest port total data: ", counter_data)
print("New port: ", counter_new)

if os.info() == "Linux" then
	-- copy portmaster.db to timestamp
	os.execute('cp -f '..DB_PATH..' '..data_timestamp..'-portmaster.db')

	-- if counter_new > 0 then overwrite new-portmaster.db with new-portmaster.db.tmp
	if counter_new > 0 then
		os.execute('mv -f new-portmaster.db.tmp '..NEW_DB_PATH)
	end
else
	-- copy portmaster.db to timestamp
	os.execute('copy /Y '..DB_PATH..' '..data_timestamp..'-portmaster.db')

	-- if counter_new > 0 then overwrite new-portmaster.db with new-portmaster.db.tmp
	if counter_new > 0 then
		if fileExists(NEW_DB_PATH) then os.remove(NEW_DB_PATH) end
		os.execute('ren new-portmaster.db.tmp '..NEW_DB_PATH)
	end
end
if fileExists('ports.md') then os.remove('ports.md') end
-- os.execute('pause')