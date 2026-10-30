namespace Chickin.Web.Services;

public sealed class UartOptions
{
    public const string SectionName = "UART";

    public string PortName { get; set; } = string.Empty;

    public int BaudRate { get; set; } = 115200;

    public int ReadTimeoutMilliseconds { get; set; } = 3000;
}
