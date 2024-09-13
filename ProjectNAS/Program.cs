using ProjectNAS.Services;

var builder = WebApplication.CreateBuilder(args);

// Add services to the container.
builder.Services.AddControllersWithViews();
builder.Services.AddRazorPages();
builder.Services.AddServerSideBlazor(); // This registers services needed for Blazor Server
builder.Services.AddSingleton<FileService>(); // Register your FileService

var app = builder.Build();

// Configure the HTTP request pipeline.
if (!app.Environment.IsDevelopment())
{
    app.UseExceptionHandler("/Home/Error");
    app.UseHsts(); // Optional: adds HTTP Strict Transport Security
}

app.UseHttpsRedirection();
app.UseStaticFiles();

app.UseRouting();

app.UseAuthorization(); // Ensure that authorization is enabled if needed

app.UseEndpoints(endpoints =>
{
    endpoints.MapControllerRoute(
        name: "default",
        pattern: "{controller=Home}/{action=Index}/{id?}");

    // Map Razor Pages
    endpoints.MapRazorPages();
    endpoints.MapBlazorHub(); // Map Blazor Server Hub

});


app.MapFallbackToPage("/_Host"); // Fallback to _Host.cshtml for Blazor routing

app.Run();
