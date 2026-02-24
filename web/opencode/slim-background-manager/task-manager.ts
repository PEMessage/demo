import { exec, ChildProcess } from 'child_process';
import { promisify } from 'util';
import * as fs from 'fs/promises';
import * as path from 'path';

const execAsync = promisify(exec);

export interface BackgroundTask {
  id: string;
  description: string;
  command: string;
  status: "pending" | "running" | "completed" | "failed";
  result?: string;
  error?: string;
  startedAt: Date;
  completedAt?: Date;
  pid?: number;
}

export interface LaunchOptions {
  command: string;
  description: string;
  cwd?: string;
  env?: Record<string, string>;
}

function generateTaskId(): string {
  return `task_${Math.random().toString(36).substring(2, 10)}`;
}

export class BackgroundTaskManager {
  private tasks = new Map<string, BackgroundTask>();
  private processes = new Map<string, ChildProcess>();
  private pollInterval?: ReturnType<typeof setInterval>;
  private workDir: string;

  constructor(workDir: string = process.cwd()) {
    this.workDir = workDir;
  }

  async launch(opts: LaunchOptions): Promise<BackgroundTask> {
    const task: BackgroundTask = {
      id: generateTaskId(),
      description: opts.description,
      command: opts.command,
      status: "running",
      startedAt: new Date(),
    };
    const task_id = task.id

    this.tasks.set(task.id, task);
    this.startPolling();

    // 执行shell命令
    const childProcess = exec(
      opts.command,
      {
        cwd: opts.cwd || this.workDir,
        env: { ...process.env, ...opts.env },
      },
      (error, stdout, stderr) => {
        const task = this.tasks.get(task_id);
        if (!task) return;

        if (error) {
          task.status = "failed";
          task.error = stderr || error.message;
        } else {
          task.status = "completed";
          task.result = stdout;
        }
        task.completedAt = new Date();
        this.processes.delete(task.id);
      }
    );

    if (childProcess.pid) {
      task.pid = childProcess.pid;
      this.processes.set(task.id, childProcess);
    }

    return task;
  }

  async getResult(taskId: string, block = false, timeout = 120000): Promise<BackgroundTask | null> {
    const task = this.tasks.get(taskId);
    if (!task) return null;

    if (!block || task.status === "completed" || task.status === "failed") {
      return task;
    }

    const deadline = Date.now() + timeout;
    while (Date.now() < deadline) {
      if (task.status === "completed" || task.status === "failed") {
        return task;
      }
      await new Promise((r) => setTimeout(r, 1000));
    }

    return task;
  }

  cancel(taskId?: string): number {
    if (taskId) {
      const task = this.tasks.get(taskId);
      const process = this.processes.get(taskId);

      if (task && task.status === "running") {
        if (process) {
          process.kill();
          this.processes.delete(taskId);
        }
        task.status = "failed";
        task.error = "Cancelled by user";
        task.completedAt = new Date();
        return 1;
      }
      return 0;
    }

    let count = 0;
    for (const [id, task] of this.tasks.entries()) {
      if (task.status === "running") {
        const process = this.processes.get(id);
        if (process) {
          process.kill();
          this.processes.delete(id);
        }
        task.status = "failed";
        task.error = "Cancelled by user";
        task.completedAt = new Date();
        count++;
      }
    }
    return count;
  }

  listTasks(status?: BackgroundTask["status"]): BackgroundTask[] {
    const tasks = [...this.tasks.values()];
    if (status) {
      return tasks.filter(t => t.status === status);
    }
    return tasks;
  }

  getTask(taskId: string): BackgroundTask | undefined {
    return this.tasks.get(taskId);
  }

  async cleanCompleted(): Promise<number> {
    let count = 0;
    for (const [id, task] of this.tasks.entries()) {
      if (task.status === "completed" || task.status === "failed") {
        this.tasks.delete(id);
        count++;
      }
    }
    return count;
  }

  private startPolling() {
    if (this.pollInterval) return;

    this.pollInterval = setInterval(() => {
      const runningTasks = [...this.tasks.values()].filter((t) => t.status === "running");

      if (runningTasks.length === 0 && this.pollInterval) {
        clearInterval(this.pollInterval);
        this.pollInterval = undefined;
      }
    }, 2000);
  }
}
