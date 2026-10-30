using System.IO.Ports;
using System.Text;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Options;

namespace Chickin.Web.Services;

public sealed class UartService : IUartService, IDisposable
{
    private const string GetStatusCommand = "GET_STATUS";
    private const string OpenCommand = "OPEN";
    private const string CloseCommand = "CLOSE";
    private const string Acknowledgement = "ACK";
    private const string LineEnding = "\r\n";

    private readonly UartOptions _options;
    private readonly ILogger<UartService> _logger;
    private readonly SemaphoreSlim _commandLock = new(1, 1);
    private SerialPort? _serialPort;
    private bool _disposed;

    public UartService(IOptions<UartOptions> options, ILogger<UartService> logger)
    {
        _options = options.Value;
        _logger = logger;

        _logger.LogInformation(
            "UART service configured: port {PortName}, baud rate {BaudRate}, read/write timeout {ReadTimeoutMilliseconds} ms, 8N1, no handshake.",
            _options.PortName,
            _options.BaudRate,
            _options.ReadTimeoutMilliseconds);
    }

    public async Task<string> GetStatusAsync(CancellationToken cancellationToken = default)
    {
        var response = await SendCommandAsync(GetStatusCommand, cancellationToken);
        const string prefix = "ACK ";

        if (!response.StartsWith(prefix, StringComparison.Ordinal) || response.Length == prefix.Length)
        {
            throw new UartCommandException(response);
        }

        return response[prefix.Length..];
    }

    public async Task OpenAsync(CancellationToken cancellationToken = default)
    {
        await SendAcknowledgedCommandAsync(OpenCommand, cancellationToken);
    }

    public async Task CloseAsync(CancellationToken cancellationToken = default)
    {
        await SendAcknowledgedCommandAsync(CloseCommand, cancellationToken);
    }

    private async Task SendAcknowledgedCommandAsync(string command, CancellationToken cancellationToken)
    {
        var response = await SendCommandAsync(command, cancellationToken);

        if (!string.Equals(response, Acknowledgement, StringComparison.Ordinal))
        {
            throw new UartCommandException(response);
        }
    }

    private async Task<string> SendCommandAsync(string command, CancellationToken cancellationToken)
    {
        _logger.LogDebug("Waiting to send UART command {Command}.", command);
        await _commandLock.WaitAsync(cancellationToken);

        try
        {
            ObjectDisposedException.ThrowIf(_disposed, this);

            return await Task.Run(() =>
            {
                var port = GetOrOpenPort();

                try
                {
                    _logger.LogDebug("Sending UART command {Command}.", command);
                    port.Write(command + LineEnding);
                    var response = port.ReadLine();
                    _logger.LogDebug("Received UART response to {Command}: {Response}.", command, response);
                    return response;
                }
                catch (Exception exception)
                {
                    _logger.LogError(exception, "UART transaction failed for command {Command} on {PortName}.", command, _options.PortName);
                    port.Dispose();
                    _serialPort = null;
                    throw;
                }
            }, CancellationToken.None);
        }
        finally
        {
            _commandLock.Release();
        }
    }

    private SerialPort GetOrOpenPort()
    {
        if (_serialPort is { IsOpen: true })
        {
            return _serialPort;
        }

        var port = new SerialPort(_options.PortName, _options.BaudRate, Parity.None, 8, StopBits.One)
        {
            Encoding = Encoding.ASCII,
            NewLine = LineEnding,
            ReadTimeout = _options.ReadTimeoutMilliseconds,
            WriteTimeout = _options.ReadTimeoutMilliseconds,
            Handshake = Handshake.None
        };

        try
        {
            _logger.LogDebug("Opening UART port {PortName}.", _options.PortName);
            port.Open();
            _serialPort = port;
            _logger.LogInformation("Opened UART port {PortName}.", _options.PortName);
            return port;
        }
        catch (Exception exception)
        {
            _logger.LogError(exception, "Failed to open UART port {PortName}.", _options.PortName);
            port.Dispose();
            throw;
        }
    }

    public void Dispose()
    {
        _commandLock.Wait();

        try
        {
            if (_disposed)
            {
                return;
            }

            _disposed = true;
            _serialPort?.Dispose();
            _serialPort = null;
        }
        finally
        {
            _commandLock.Release();
        }
    }
}

public sealed class UartCommandException : IOException
{
    public UartCommandException(string response)
        : base($"UART returned: {response}")
    {
        Response = response;
    }

    public string Response { get; }
}
