namespace Chickin.Web.Services;

public sealed class MockUartService : IUartService
{
    private readonly SemaphoreSlim _commandLock = new(1, 1);
    private string _doorState = "BOTTOM";

    public async Task<string> GetStatusAsync(CancellationToken cancellationToken = default)
    {
        await _commandLock.WaitAsync(cancellationToken);

        try
        {
            return _doorState;
        }
        finally
        {
            _commandLock.Release();
        }
    }

    public async Task OpenAsync(CancellationToken cancellationToken = default)
    {
        await _commandLock.WaitAsync(cancellationToken);

        try
        {
            if (_doorState is not ("BOTTOM" or "UNKNOWN" or "TOP"))
            {
                throw new UartCommandException("ERR OPEN STATE");
            }

            _doorState = "TOP";
        }
        finally
        {
            _commandLock.Release();
        }
    }

    public async Task CloseAsync(CancellationToken cancellationToken = default)
    {
        await _commandLock.WaitAsync(cancellationToken);

        try
        {
            if (_doorState is not ("TOP" or "BOTTOM"))
            {
                throw new UartCommandException("ERR CLOSE STATE");
            }

            _doorState = "BOTTOM";
        }
        finally
        {
            _commandLock.Release();
        }
    }
}
