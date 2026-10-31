using System.Globalization;
using System.Text.Json;

namespace Chickin.Web.Services;

public sealed record DailyDoorSchedule(TimeOnly OpenTime, TimeOnly CloseTime);

public sealed class DailyDoorScheduleService : BackgroundService
{
    private readonly IUartService _uart;
    private readonly ILogger<DailyDoorScheduleService> _logger;
    private readonly SemaphoreSlim _gate = new(1, 1);
    private readonly string _schedulePath = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "Chickin",
        "door-schedule.json");

    private DailyDoorSchedule? _schedule;
    private bool _loaded;
    private DateOnly? _lastOpenDate;
    private DateOnly? _lastCloseDate;

    public DailyDoorScheduleService(IUartService uart, ILogger<DailyDoorScheduleService> logger)
    {
        _uart = uart;
        _logger = logger;
    }

    public async Task<DailyDoorSchedule?> GetScheduleAsync(CancellationToken cancellationToken = default)
    {
        await _gate.WaitAsync(cancellationToken);

        try
        {
            await LoadIfNeededAsync(cancellationToken);
            return _schedule;
        }
        finally
        {
            _gate.Release();
        }
    }

    public async Task SaveScheduleAsync(
        TimeOnly openTime,
        TimeOnly closeTime,
        CancellationToken cancellationToken = default)
    {
        if (openTime == closeTime)
        {
            throw new ArgumentException("Open and close times must be different.");
        }

        await _gate.WaitAsync(cancellationToken);

        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(_schedulePath)!);
            var storedSchedule = new StoredSchedule(
                openTime.ToString("HH:mm", CultureInfo.InvariantCulture),
                closeTime.ToString("HH:mm", CultureInfo.InvariantCulture));
            var json = JsonSerializer.Serialize(storedSchedule);
            var temporaryPath = _schedulePath + ".tmp";

            await File.WriteAllTextAsync(temporaryPath, json, cancellationToken);
            File.Move(temporaryPath, _schedulePath, overwrite: true);

            _schedule = new DailyDoorSchedule(openTime, closeTime);
            _loaded = true;
        }
        finally
        {
            _gate.Release();
        }
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        while (!stoppingToken.IsCancellationRequested)
        {
            try
            {
                var schedule = await GetScheduleAsync(stoppingToken);
                var now = DateTime.Now;
                var today = DateOnly.FromDateTime(now);

                if (schedule is not null)
                {
                    if (now.Hour == schedule.OpenTime.Hour
                        && now.Minute == schedule.OpenTime.Minute
                        && _lastOpenDate != today)
                    {
                        _lastOpenDate = today;
                        await ExecuteScheduledCommandAsync("open", _uart.OpenAsync, stoppingToken);
                    }

                    if (now.Hour == schedule.CloseTime.Hour
                        && now.Minute == schedule.CloseTime.Minute
                        && _lastCloseDate != today)
                    {
                        _lastCloseDate = today;
                        await ExecuteScheduledCommandAsync("close", _uart.CloseAsync, stoppingToken);
                    }
                }

                await Task.Delay(TimeSpan.FromSeconds(1), stoppingToken);
            }
            catch (OperationCanceledException) when (stoppingToken.IsCancellationRequested)
            {
                break;
            }
            catch (Exception exception)
            {
                _logger.LogError(exception, "Door schedule check failed.");
                await Task.Delay(TimeSpan.FromSeconds(5), stoppingToken);
            }
        }
    }

    private async Task ExecuteScheduledCommandAsync(
        string commandName,
        Func<CancellationToken, Task> command,
        CancellationToken cancellationToken)
    {
        try
        {
            _logger.LogInformation("Executing scheduled door {Command} command.", commandName);
            await command(cancellationToken);
        }
        catch (Exception exception)
        {
            _logger.LogError(exception, "Scheduled door {Command} command failed.", commandName);
        }
    }

    private async Task LoadIfNeededAsync(CancellationToken cancellationToken)
    {
        if (_loaded)
        {
            return;
        }

        if (File.Exists(_schedulePath))
        {
            var json = await File.ReadAllTextAsync(_schedulePath, cancellationToken);
            var storedSchedule = JsonSerializer.Deserialize<StoredSchedule>(json)
                ?? throw new InvalidDataException("The saved door schedule is empty.");

            if (!TimeOnly.TryParseExact(storedSchedule.OpenTime, "HH:mm", CultureInfo.InvariantCulture, DateTimeStyles.None, out var openTime)
                || !TimeOnly.TryParseExact(storedSchedule.CloseTime, "HH:mm", CultureInfo.InvariantCulture, DateTimeStyles.None, out var closeTime)
                || openTime == closeTime)
            {
                throw new InvalidDataException("The saved door schedule contains invalid times.");
            }

            _schedule = new DailyDoorSchedule(openTime, closeTime);
        }

        _loaded = true;
    }

    private sealed record StoredSchedule(string OpenTime, string CloseTime);
}
