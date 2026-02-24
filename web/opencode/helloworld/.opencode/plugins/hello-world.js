// How to check log:
// 1. opencode --log-level DEBUG --print-logs web
// 2. find it in ~/.local/share/opencode/log/
// How to install this plugin (local way)
// 1. put it in ~/.config/opencode/plugins dir
// 2. put it in project .opencode dir: .opencode/plugins/


export const HelloWorldPlugin = async ({ project, client, $, directory, worktree }) => {
  await client.app.log({
    body: {
      service: "hello-world-plugin",
      level: "info",
      message: "Hello World plugin initialized!",
      extra: { directory, worktree },
    },
  });

  return {
    "session.created": async (input, output) => {
      await client.app.log({
        body: {
          service: "hello-world-plugin",
          level: "info",
          message: "New session created",
        },
      });
    },
  };
};
