extern crate time;
extern crate toml;

use std::process::*;
use std::env;
use std::fs;
use std::path::Path;
use std::thread;
use std::io::{Read, Write};

mod config;

fn nice_size(size: u64) -> String {
    if size >= 1024 * 1024 {
        format!("{:6.2} MiB", (size as f64) / 1024.0 / 1024.0)
    } else if size >= 1024 {
        format!("{:6.2} KiB", (size as f64) / 1024.0)
    } else {
        format!("{:6.2} B  ", (size as f64))
    }
}

fn load_config(arg: &str) -> config::Config {
    let mut toml = String::new();
    fs::File::open(&arg)
        .ok()
        .expect("config file not found")
        .read_to_string(&mut toml)
        .unwrap();
    let toml = toml::Parser::new(&toml)
        .parse()
        .expect("config file not in valid TOML format");

    config::parse_config(toml)
        .ok()
        .expect("Invalid config file")
}

// type, value, time, size, compsize, compratio
type Line<'a> = (&'a str, &'a str, f64, &'a str, &'a str, f64);
type LinePad = (usize,);

fn print_header(kind: &str, value: &str, padding: LinePad) {
    println!("{:4} | {:6$} | {:^9} | {:^12} | {:^12} | {:^8}",
        kind, value, "time", "size", "comp. size", "ratio", padding.0);
}

fn print_line(line: Line, padding: LinePad) {
    println!("{:4} | {:6$} | {:7.3} s | {:>12} | {:>12} | {:6.2} %",
        line.0, line.1, line.2, line.3, line.4, line.5, padding.0);
}

fn print_sep(padding: LinePad) {
    println!("{:-^4}---{:-^6$}---{:-^7}-----{:-^12}---{:-^12}---{:-^6}--",
        "", "", "", "", "", "", padding.0);
}

// let mem_cmd = r##"valgrind --tool=massif --pages-as-heap=yes --massif-out-file=${mofile} ${cmd}; grep mem_heap_B ${mofile} | sed -e 's/mem_heap_B=\(.*\)/\1/' | sort -g | tail -n 1"##;

fn bash_expand(s: &str) -> String {
    let mut cmd = Command::new("bash");
    cmd.stdin(Stdio::piped());
    cmd.stdout(Stdio::piped());
    let mut handles = cmd.spawn().unwrap();

    let mut stdin = handles.stdin.take().unwrap();
    stdin.write_all("echo ".as_bytes()).unwrap();
    stdin.write_all(s.as_bytes()).unwrap();
    drop(stdin);

    let out = handles.wait_with_output().unwrap();
    assert!(out.status.success());
    String::from_utf8(out.stdout).unwrap().trim().to_owned()
}

fn alphabet_size(file: &str) -> usize {
    let mut bytes = [0u64; 256];
    let file = fs::File::open(file).unwrap();
    for b in file.bytes() {
        bytes[b.unwrap() as usize] += 1;
    }
    let mut counter = 0;
    for &count in &bytes[..] {
        if count > 0 {
            counter += 1;
        }
    }
    counter
}

fn main() {
    let arg = &env::args().nth(1).expect("need to be given a filename");
    let profile_name = &env::args().nth(2).expect("need to be given a profile");

    let config = load_config(&arg);
    let profile = &config.profiles[profile_name];

    let inputs = profile.inputs.iter().map(|input| {
        (input, fs::metadata(input).ok().expect(&format!("input file '{}' does not exist", input)).len())
    }).collect::<Vec<_>>();

    let input = inputs[0].0;
    let input_size = inputs[0].1;

    let commands = &profile.commands;
    let commands: Vec<_> = commands.iter().map(|&(ref cmd, ref cargs, ref ending)| {
        let command_output = format!("{}.{}",
            Path::new(input).file_name().unwrap().to_str().unwrap()
            , ending);
        let c = format!("{} {}", cmd, cargs);
        let command = c.replace("${1}", input)
                       .replace("${2}", &command_output);
        (command, command_output, &cmd[..])
    }).collect();

    let headers = inputs.iter().map(|x| &x.0);
    let rows = commands.iter().map(|x| &x.2);

    let padding = headers.map(|s| s.len())
                         .chain(rows.map(|s| s.len()))
                         .max()
                         .unwrap_or(0);
    let padding = (padding,);

    let run_row = |kind: &str, command: &str, file: &str, value: &str| {
        let mut cmd = Command::new("bash");
        cmd.arg("-c").arg(&format!("rm {}", file));
        cmd.output().unwrap();

        let run = command;
        let mut cmd = Command::new("bash");
        cmd.arg("-c").arg(run);
        let time_start = time::precise_time_ns();
        cmd.output().ok().expect("command not successful");
        let time_end = time::precise_time_ns();
        let time_span = ((time_end - time_start) as f64)
            / 1000.0
            / 1000.0
            / 1000.0;

        let out_size = fs::metadata(file).unwrap().len();
        let cmd = value;
        let comp_size = &nice_size(out_size);
        let inp_size = &nice_size(input_size);
        let ratio = (out_size as f64) / (input_size as f64) * 100.0;
        print_line(
            (kind, cmd, time_span, inp_size, comp_size, ratio),
            padding
        );
    };

    if profile.compare_commands {

    } else {

    }

    print_header("file", &input, padding);
    print_sep(padding);
    for command in &commands {
        run_row("cmd", &command.0, &command.1, &command.2);
    }

}
