const http = require("http");
const { execFileSync } = require("child_process");

const VITE_URL = "http://localhost:5173";
const POLL_MS = 200;

function check() {
  return new Promise((resolve) => {
    http.get(VITE_URL, () => resolve(true)).on("error", () => resolve(false));
  });
}

(async () => {
  while (!(await check())) await new Promise((r) => setTimeout(r, POLL_MS));
  execFileSync(require("electron"), ["."], { stdio: "inherit" });
})();
