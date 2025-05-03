local cmd = {}

-- PostBuildCommands 中命令返回值为 0 才算成功，robocopy 的返回值需要特殊处理
-- https://learn.microsoft.com/en-us/troubleshoot/windows-server/backup-and-storage/return-codes-used-robocopy-utility
-- Any value greater than or equal to 8 indicates that there was at least one failure during the copy operation.

-- robocopy 会把 \" 转义为双引号
-- MSBuild 提供的 Macro 例如 $(SolutionDir) 是以 \ 结尾的，所以在 robocopy 中使用 "$(SolutionDir)" 时，最后一个双引号会被转义，导致路径错误

function cmd.copy_file_to_build_target_dir(file_path)
    local src_dir = path.getdirectory(file_path)
    local filename = path.getname(file_path)
    return string.format('robocopy %%[%s] %%[%%{!cfg.targetdir}] "%s" /XO /XX & if %%ERRORLEVEL%% LSS 8 (set ERRORLEVEL=0)', src_dir, filename)
end

function cmd.mirror_dir_to_build_target_dir(dir_path, ...)
    local dest_dir = path.join("%{!cfg.targetdir}", ...)
    return string.format("robocopy %%[%s] %%[%s] /MIR & if %%ERRORLEVEL%% LSS 8 (set ERRORLEVEL=0)", dir_path, dest_dir)
end

return cmd