using ClipwatchSharp;

using var clipwatch = new ClipboardWatcher();
clipwatch.ClipboardChanged += (sender, s) =>
{
    Console.WriteLine($"EVENT: {s}");
};
Console.WriteLine("Started in stopped state");

for (int i = 0; i < 3; ++i)
{
    Thread.Sleep(5000);
    clipwatch.Start();
    Thread.Sleep(5000);
    clipwatch.Stop();
    Console.WriteLine("Stopped");
}