$RemoteUserName='trieun'
$RemoteHostName='eve.eecs.oregonstate.edu'
$PrivateKey='D:\EvePrivatekey.ppk'
$SolutionDir=$PWD
$RemoteWorkingDir='/scratch/nini/npsi_2020/npsi_my'

# only files with these exptenstions will be copied
$FileMasks='**.cpp;**.c;**.h;*CMakeLists.txt;*.bin;*.S;thirdparty/linux/**.get'

# everything in these folders will be skipped
$ExcludeDirs='.git/;thirdparty/;Debug/;Release/;x64/;ipch/;.vs/'

C:\WinSCP\WinSCP.com  /command `
    "open $RemoteUserName@$RemoteHostName -privatekey=""$PrivateKey"""`
    "call mkdir -p $RemoteWorkingDir"`
    "synchronize Remote $SolutionDir $RemoteWorkingDir -filemask=""$FileMasks|$ExcludeDirs;"""`
    "exit" 