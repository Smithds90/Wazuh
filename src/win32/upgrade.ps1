# Check argument and perform background execution if "B" is passed
if ($args[0] -eq "B") {
  # Simulate 5 second wait (alternatives explained later)
  Start-Sleep -Seconds 5

  # Start do_upgrade.ps1 in a separate process with Bypass execution policy
  Start-Process PowerShell -ArgumentList "-NoProfile", "-ExecutionPolicy Bypass", "-File", ".\\do_upgrade.ps1" -Wait

  exit
}

# Copy files (assuming upgrade folder is in the same directory)
Copy-Item -Path "upgrade\upgrade.ps1", -Destination ".", -Force
Copy-Item -Path "upgrade\do_upgrade.ps1", -Destination ".", -Force
Copy-Item -Path "upgrade\wazuh-agent-*.msi", -Destination ".", -Force

# Run upgrade.ps1 with Bypass execution policy
Start-Process -FilePath ".\\upgrade.ps1" -ArgumentList "B" -Wait

# Cleanup (assuming script doesn't need upgrade.ps1 anymore)
Remove-Item -Path ".\\upgrade.ps1" -Force
