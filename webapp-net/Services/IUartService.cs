namespace Chickin.Web.Services;

public interface IUartService
{
    Task<string> GetStatusAsync(CancellationToken cancellationToken = default);

    Task OpenAsync(CancellationToken cancellationToken = default);

    Task CloseAsync(CancellationToken cancellationToken = default);
}
