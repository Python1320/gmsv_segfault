if not system.IsLinux() then return end

timer.Simple(0,function()
        require'segfault'
end)
