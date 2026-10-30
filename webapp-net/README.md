# Chickin web app

Door states: [doorstatemachine.cpp](../door/doorstatemachine.cpp).

## Orange Pi Zero 3

Check OS:

```sh
dpkg --print-architecture
cat /etc/os-release
```

Debian 12 ARM64 runtime:

```sh
wget https://packages.microsoft.com/config/debian/12/packages-microsoft-prod.deb
sudo dpkg -i packages-microsoft-prod.deb
rm packages-microsoft-prod.deb
sudo apt update
sudo apt install -y aspnetcore-runtime-10.0
```

Publish from Windows x64 using the `orangepi-arm64` profile:

```powershell
dotnet publish .\Chickin.Web.csproj -p:PublishProfile=orangepi-arm64
```

Output: `bin\publish\orangepi-arm64\`. Copy its contents to `/home/orangepi/webapp-net/`. Set `UART:PortName` in `appsettings.json`.

## systemd

```sh
sudo cp chickin-web.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now chickin-web.service
sudo journalctl -u chickin-web.service -f
```

App: `http://<orange-pi-ip>/` (HTTP on port 80; HTTPS is not configured).

UART debug logs: `sudo journalctl -u chickin-web.service -f`.
