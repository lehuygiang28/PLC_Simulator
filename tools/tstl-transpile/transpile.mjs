/**
 * PLC Simulator — TypeScript → Lua transpile CLI.
 * Usage: node transpile.mjs <input.ts> [--out <output.lua>]
 * Exit 0 + Lua on stdout (or --out file). Diagnostics on stderr, exit 1 on failure.
 */
import { readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import ts from "typescript";
import { LuaLibImportKind, LuaTarget, transpileString } from "typescript-to-lua";

const __dirname = dirname(fileURLToPath(import.meta.url));

function usage() {
  process.stderr.write(
    "Usage: node transpile.mjs <input.ts> [--out <output.lua>]\n"
  );
}

function parseArgs(argv) {
  const positional = [];
  let outPath = null;
  for (let i = 0; i < argv.length; ++i) {
    if (argv[i] === "--out" && i + 1 < argv.length) {
      outPath = argv[++i];
    } else {
      positional.push(argv[i]);
    }
  }
  return { inputPath: positional[0] ?? null, outPath };
}

function formatDiagnostic(diag) {
  if (diag.file && diag.start !== undefined) {
    const { line, character } = diag.file.getLineAndCharacterOfPosition(
      diag.start
    );
    const fileName = diag.file.fileName.replace(/\\/g, "/");
    const base = fileName.split("/").pop() ?? fileName;
    return `${base}(${line + 1},${character + 1}): ${ts.flattenDiagnosticMessageText(
      diag.messageText,
      "\n"
    )}`;
  }
  return ts.flattenDiagnosticMessageText(diag.messageText, "\n");
}

const { inputPath, outPath } = parseArgs(process.argv.slice(2));
if (!inputPath) {
  usage();
  process.exit(2);
}

const apiDeclPath = join(__dirname, "plc-simulator.d.ts");
const apiDecl = readFileSync(apiDeclPath, "utf8");
const userSource = readFileSync(inputPath, "utf8");

// Prepend API declarations so TSTL knows globals; user file name kept for error lines.
const virtualName = inputPath.replace(/\\/g, "/").split("/").pop() ?? "script.ts";
const combinedSource = `${apiDecl}\n// --- user script ---\n${userSource}`;

const result = transpileString(combinedSource, {
  luaTarget: LuaTarget.Lua54,
  luaLibImport: LuaLibImportKind.None,
  noHeader: true,
});

const errors = (result.diagnostics ?? []).filter(
  (d) => d.category === ts.DiagnosticCategory.Error
);

if (errors.length > 0) {
  process.stderr.write(errors.map(formatDiagnostic).join("\n"));
  process.stderr.write("\n");
  process.exit(1);
}

const lua = result.file?.lua ?? "";
if (outPath) {
  writeFileSync(outPath, lua, "utf8");
} else {
  process.stdout.write(lua);
}
