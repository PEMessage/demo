import { BackgroundTaskManager, BackgroundTask } from './task-manager';
import * as readline from 'readline';

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});

const manager = new BackgroundTaskManager(process.cwd());

const menu = `
╔══════════════════════╗
║  Task Manager        ║
╠══════════════════════╣
║ 1. New task          ║
║ 2. List all          ║
║ 3. List running      ║
║ 4. Check task        ║
║ 5. Cancel task       ║
║ 6. Cancel all        ║
║ 7. Clean done        ║
║ 0. Exit              ║
╚══════════════════════╝
`;

const ask = (q: string): Promise<string> =>
  new Promise(resolve => rl.question(q, resolve));

async function main() {
  console.log('🚀 Task Manager Started');

  while (true) {
    console.log(menu);
    const choice = await ask('Choose: ');

    switch (choice) {
      case '1': {
        const cmd = await ask('Command: ');
        const desc = await ask('Description: ') || cmd;
        const task = await manager.launch({ command: cmd, description: desc });
        console.log(`✅ Started #${task.id}: ${task.command}`);
        break;
      }

      case '2':
        printTasks(manager.listTasks(), 'All tasks');
        break;

      case '3':
        printTasks(manager.listTasks('running'), 'Running tasks');
        break;

      case '4': {
        const id = await ask('Task ID: ');
        const wait = (await ask('Wait? (y/n): ')).toLowerCase() === 'y';
        const task = await manager.getResult(id, wait, wait ? 30000 : 0);
        task ? printTask(task) : console.log('❌ Not found');
        break;
      }

      case '5': {
        const id = await ask('Task ID: ');
        console.log(manager.cancel(id) ? '✅ Cancelled' : '❌ Not found');
        break;
      }

      case '6':
        console.log(`✅ Cancelled ${manager.cancel()} tasks`);
        break;

      case '7':
        console.log(`✅ Cleaned ${await manager.cleanCompleted()} tasks`);
        break;

      case '0':
        console.log('👋 Bye!');
        process.exit(0);

      default:
        console.log('❌ Invalid choice');
    }
  }
}

function printTasks(tasks: BackgroundTask[], title: string) {
  console.log(`\n${title} (${tasks.length}):`);
  tasks.length ? tasks.forEach(printTask) : console.log(' None');
}

function printTask(t: BackgroundTask) {
  console.log(`
######: ${t.id} [${t.status}]
  Cmd: ${t.command}
  Desc: ${t.description}
  ${t.pid ? `PID: ${t.pid}` : ''}
  ${t.error ? `Err: ${t.error}` : ''}
  ${t.result ? `Out: ${t.result}` : ''}
  `);
}

process.on('SIGINT', () => {
  console.log('\n👋 Shutting down...');
  manager.cancel();
  process.exit();
});

main().catch(console.error);
