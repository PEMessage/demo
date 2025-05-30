// See:
// https://aka.ms/new-console-template for more information
// https://learn.microsoft.com/en-us/dotnet/api/?view=net-9.0

// run this file using dotnet run

// Step. 0
// =====================
// Hello world
Console.WriteLine("Hello, World!");

// Step. 1
// =====================
// foreach: C++ like
// var: java like
// {: C like
foreach (var arg in args) {
    Console.WriteLine(arg);
}


// Step. 2
// =====================
for ( int i = 0; i < args.Length ; i++ ) {
    // format language like python
    Console.WriteLine("{0} args is: {1}", i, args[i]);
    Console.WriteLine($"{i} args is: {args[i]} (format string)");
}



// Step. 3
// =====================
// Classic what's your name
/* while (true) { */
#pragma warning disable CS0162 // Unreachable code detected
while (false) {
    string? name = null;
    Console.Write("What's your name: ");
    name = Console.ReadLine();
    if ( name != null ) {
        Console.WriteLine("Your name is: {0}", name);
    } else {
        Console.WriteLine("\nEmtry input!", name);
        break;
    }
}
#pragma warning restore CS0162

// Step. 4.1 Where
// =====================
// Lambda: Javascript like
// Lambda表达式可以用作“thunk, 即延迟执行的函数
// Thunk: a value yet to be evaluated
// LinQ: also relate to lazy eval
//
// python: even = [x for x in range(1, 101) if x % 2 == 0]
//         or even = list(filter(lambda x: x % 2 == 0, range(1, 101)))
//         python range(1,10) == [1, 2 ... 9]
//         csharp range(1,10) == [1, 2 ... 9, 10]
var even = Enumerable.Range(1, 10).Where((x) => x % 2 == 0);
foreach (var item in even) {
    Console.WriteLine($"{item}");
}

// Step. 4.2 Select
// =====================
/* var even = Enumerable.Range(1, 100).Where((x) => x % 2 == 0); */
