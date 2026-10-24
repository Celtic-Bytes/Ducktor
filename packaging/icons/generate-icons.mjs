// Generates the platform icons from resources/icons/ducktor.svg, the single source of the logo.
// Usage (in packaging/icons): npm ci && npm run generate
import { Resvg } from "@resvg/resvg-js";
import { mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, "..", "..");
const svg = readFileSync(join(root, "resources", "icons", "ducktor.svg"));

const cache = new Map();
function png(size) {
  if (!cache.has(size)) {
    const image = new Resvg(svg, { fitTo: { mode: "width", value: size } }).render();
    cache.set(size, image.asPng());
  }
  return cache.get(size);
}

// Linux: hicolor PNG sizes next to the scalable SVG.
const linuxSizes = [16, 24, 32, 48, 64, 128, 256, 512];
const pngDir = join(here, "png");
mkdirSync(pngDir, { recursive: true });
for (const size of linuxSizes) writeFileSync(join(pngDir, `ducktor-${size}.png`), png(size));

// Windows: ICO with PNG-compressed entries (supported since Windows Vista).
function ico(sizes) {
  const images = sizes.map(png);
  const header = Buffer.alloc(6 + 16 * sizes.length);
  header.writeUInt16LE(0, 0);
  header.writeUInt16LE(1, 2);
  header.writeUInt16LE(sizes.length, 4);
  let offset = header.length;
  sizes.forEach((size, i) => {
    const entry = 6 + 16 * i;
    header.writeUInt8(size >= 256 ? 0 : size, entry);
    header.writeUInt8(size >= 256 ? 0 : size, entry + 1);
    header.writeUInt16LE(1, entry + 4);
    header.writeUInt16LE(32, entry + 6);
    header.writeUInt32LE(images[i].length, entry + 8);
    header.writeUInt32LE(offset, entry + 12);
    offset += images[i].length;
  });
  return Buffer.concat([header, ...images]);
}
writeFileSync(join(root, "packaging", "windows", "ducktor.ico"), ico([16, 24, 32, 48, 64, 128, 256]));

// macOS: ICNS with PNG entries; the @2x types reuse the double-size image.
function icns(entries) {
  const chunks = entries.map(([type, size]) => {
    const data = png(size);
    const head = Buffer.alloc(8);
    head.write(type, 0, "ascii");
    head.writeUInt32BE(data.length + 8, 4);
    return Buffer.concat([head, data]);
  });
  const body = Buffer.concat(chunks);
  const head = Buffer.alloc(8);
  head.write("icns", 0, "ascii");
  head.writeUInt32BE(body.length + 8, 4);
  return Buffer.concat([head, body]);
}
writeFileSync(
  join(root, "packaging", "macos", "ducktor.icns"),
  icns([
    ["icp4", 16], ["icp5", 32], ["icp6", 64], ["ic07", 128], ["ic08", 256], ["ic09", 512], ["ic10", 1024],
    ["ic11", 32], ["ic12", 64], ["ic13", 256], ["ic14", 512],
  ]),
);

console.log("Icons written to packaging/icons/png, packaging/windows/ducktor.ico and packaging/macos/ducktor.icns");
