#! /usr/bin/env node

const path = require('path');
const { execSync } = require('child_process');
const fs = require('fs');

const convertor = '../node_modules/.bin/lv_font_conv';
const roboto = '../node_modules/roboto-fontface/fonts/roboto/Roboto-Medium.woff';
const icons = 'icons.ttf';
const outdir = '../src/fonts';


let out_file;
let bpp = 3;
let compression = '';

if (process.argv[2] == 'nocompress') {
  bpp = 4;
  compression = '--no-compress';
}

const commons = `--bpp ${bpp} ${compression} --lcd --format lvgl --lv-include lvgl.h --force-fast-kern-format`;

out_file = path.join(outdir, 'my_font_roboto_12.c');
console.log(`Build ${path.join(__dirname, out_file)}`);
execSync(`${convertor} ${commons} --size 12 --font ${roboto} --range 0x20-0x7F --symbols ³ --font ${icons} --range 0xE006 -o ${out_file}`, { stdio: 'inherit', cwd: __dirname });

out_file = path.join(outdir, 'my_font_roboto_14.c');
console.log(`Build ${path.join(__dirname, out_file)}`);
execSync(`${convertor} ${commons} --size 14 --font ${roboto} --range 0x20-0x7F --symbols ³ --font ${icons} --range 0xE006 -o ${out_file}`, { stdio: 'inherit', cwd: __dirname });

out_file = path.join(outdir, 'my_font_roboto_num_18.c');
console.log(`Build ${path.join(__dirname, out_file)}`);
execSync(`${convertor} ${commons} --size 18 --font ${roboto} --symbols 0123456789. --font ${icons} --range 0xE003,0xE004 --range '0xE005=>0x78' -o ${out_file}`, { stdio: 'inherit', cwd: __dirname });

out_file = path.join(outdir, 'my_font_icons_18.c');
console.log(`Build ${path.join(__dirname, out_file)}`);
execSync(`${convertor} ${commons} --size 18 --font ${icons} --range 0xE000,0xE001,0xE002 -o ${out_file}`, { stdio: 'inherit', cwd: __dirname });
