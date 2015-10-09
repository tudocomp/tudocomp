extern crate time;
extern crate toml;

use std::process::*;
use std::env;
use std::fs;
use std::path::Path;
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

/// Run the argument as a bash script
fn bash_run(command_line: &str) -> Output {
    let mut cmd = Command::new("sh");
    cmd.stdin(Stdio::piped());
    cmd.stdout(Stdio::piped());
    cmd.stderr(Stdio::piped());
    let mut handles = cmd.spawn().unwrap();

    let mut stdin = handles.stdin.take().unwrap();
    stdin.write_all(command_line.as_bytes()).unwrap();
    drop(stdin);

    handles.wait_with_output().unwrap()
}

/// Expand the argument through bash, substituting environment variables.
fn bash_expand(s: &str) -> String {
    let out = bash_run(&format!("echo {}", s));
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

/// Data that describes a single input file
struct InputData {
    unexpanded: String,
    expanded: String,
    size: u64,
}

/// Data that describes a single command
struct CommandData {
    unexpanded_visible: String,
    unexpanded_hidden: String,
    output_extension: String,
}

fn bash_in_out_script(input: &str, output: &str, command: &str) -> String {
    format!("IN={}
             OUT={}
             {}", input, output, command)
}

fn file_name(file: &str) -> &str {
    Path::new(file).file_name().unwrap().to_str().unwrap()
}

fn main() {
    let arg = &env::args().nth(1).expect("need to be given a filename");
    let profile_name = &env::args().nth(2).expect("need to be given a profile");

    let config = load_config(&arg);
    let profile = &config.profiles[profile_name];

    let inputs = profile.inputs.iter().map(|input| {
        let input_expanded = bash_expand(&input);
        let input_size = fs::metadata(&input_expanded)
            .ok()
            .expect(&format!("input file '{}' does not exist", input_expanded))
            .len();
        InputData {
            unexpanded: input.clone(),
            expanded: input_expanded,
            size: input_size,
        }
    }).collect::<Vec<_>>();


    let commands: Vec<_> = profile.commands.iter().map(
        |&(ref cmd, ref cargs, ref ending)|{
            CommandData {
                unexpanded_visible: cmd.clone(),
                unexpanded_hidden: cargs.clone(),
                output_extension: ending.clone(),
            }
        }
    ).collect();

    let headers = inputs.iter().map(|x| &x.unexpanded);
    let rows = commands.iter().map(|x| &x.unexpanded_visible);

    let padding = headers.map(|s| s.len())
                         .chain(rows.map(|s| s.len()))
                         .max()
                         .unwrap_or(0);
    let padding = (padding,);

    let run_row = |kind: &str, label: &str, command: &CommandData, input: &InputData| {
        let output = &(
            file_name(&input.expanded).to_owned()
            + "."
            + &command.output_extension
        );

        bash_run(&format!("rm {}", output));

        let script = &bash_in_out_script(&input.expanded, output,
            &format!("{} {}", command.unexpanded_visible,
                              command.unexpanded_hidden)
        );

        let time_start = time::precise_time_ns();
        let out = bash_run(script);
        assert!(out.status.success());
        let time_end = time::precise_time_ns();

        let time_span = ((time_end - time_start) as f64)
            / 1000.0
            / 1000.0
            / 1000.0;

        let out_size = fs::metadata(output).unwrap().len();
        let comp_size = &nice_size(out_size);
        let inp_size = &nice_size(input.size);
        let ratio = (out_size as f64) / (input.size as f64) * 100.0;
        print_line(
            (kind, label, time_span, inp_size, comp_size, ratio),
            padding
        );
    };

    println!("");
    println!("Profile: {}", profile_name);
    println!("");

    if profile.compare_commands {
        for input in &inputs {
            print_header("file", &input.unexpanded, padding);
            print_sep(padding);
            for command in &commands {
                run_row("cmd", &command.unexpanded_visible, &command, &input);
            }
            print_sep(padding);
        }
    } else {
        for command in &commands {
            print_header("cmd", &command.unexpanded_visible, padding);
            print_sep(padding);
            for input in &inputs {
                run_row("file", &input.unexpanded, &command, &input);
            }
            print_sep(padding);
        }
    }



}
