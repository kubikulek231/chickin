using Chickin.Web.Components;
using Chickin.Web.Services;
using System.Runtime.InteropServices;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddRazorComponents()
    .AddInteractiveServerComponents();

if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux)
    && RuntimeInformation.ProcessArchitecture == Architecture.Arm64)
{
    builder.Services.AddOptions<UartOptions>()
        .Bind(builder.Configuration.GetSection(UartOptions.SectionName))
        .Validate(options => !string.IsNullOrWhiteSpace(options.PortName), "UART:PortName must be configured.")
        .Validate(options => options.BaudRate > 0, "UART:BaudRate must be greater than zero.")
        .Validate(options => options.ReadTimeoutMilliseconds > 0, "UART:ReadTimeoutMilliseconds must be greater than zero.")
        .ValidateOnStart();
    builder.Services.AddSingleton<IUartService, UartService>();
}
else
{
    builder.Services.AddSingleton<IUartService, MockUartService>();
}

builder.Services.AddSingleton<DailyDoorScheduleService>();
builder.Services.AddHostedService(serviceProvider =>
    serviceProvider.GetRequiredService<DailyDoorScheduleService>());

var app = builder.Build();

if (!app.Environment.IsDevelopment())
{
    app.UseExceptionHandler("/Error", createScopeForErrors: true);
    app.UseHsts();
}
app.UseStatusCodePagesWithReExecute("/not-found", createScopeForStatusCodePages: true);

app.UseAntiforgery();

app.MapStaticAssets();
app.MapRazorComponents<App>()
    .AddInteractiveServerRenderMode();

app.Run();
