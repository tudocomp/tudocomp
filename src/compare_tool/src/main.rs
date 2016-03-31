extern crate time;
extern crate toml;
extern crate docopt;

use std::process::*;
use std::fs;
use std::path::Path;
use std::io::{Read, Write};

use docopt::Docopt;

mod config;

const USAGE: &'static str = "
Usage:
    compare_tool [-m] <config_file> <profile>
    compare_tool [--help]
    compare_tool [--version]

Options:
    -m, --with_mem    Also profile memory
        --help        This screen
        --version     Version of this tool
";

const VERSION: &'static str = "v0.1";

fn main() {
    let arguments = Docopt::new(USAGE)
        .and_then(|d| {
            d.help(true)
             .version(Some(VERSION.into()))
             .parse()
        })
        .unwrap_or_else(|e| e.exit());

    let arg = arguments.get_str("<config_file>");
    let profile_name = arguments.get_str("<profile>");

    let mut config = load_config(&arg);
    if config.profiles.get(profile_name).is_none()
    {
        let ps = config.profiles
            .iter()
            .map(|s| "- ".to_owned() + &s.0 + "\n")
            .collect::<Vec<_>>();
        let ps = ps.concat();
        panic!("need to be given one of the following profiles:\n{}", ps);
    };

    {
        let profile = config.profiles.get_mut(profile_name).unwrap();
        if arguments.get_bool("--with_mem") {
            profile.with_mem = true;
        }
    }
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

    let headers = inputs.iter().map(|x| x.unexpanded.len());
    let rows = commands.iter().map(|x| x.unexpanded_visible.len());

    let padding = headers.chain(rows).max().unwrap_or(0);
    let padding = (padding,);

    let run_row = |kind: &str, label: &str, command: &CommandData, input: &InputData| {
        let output = &(
            file_name(&input.expanded).to_owned()
            + "."
            + &command.output_extension
        );

        bash_run(&format!("rm {}", output));

        fn build_cmd(valgrind: bool, output: &str, command: &CommandData) -> String {
            let measure_pages = false;
            let max_detail = true;

            let measure_pages_opt = if measure_pages {
                " --pages-as-heap=yes "
            } else {
                ""
            };

            let max_details_opt = if max_detail {
                " --detailed-freq=1 "
            } else {
                ""
            };


            let valgrind_cmd = if valgrind {
                format!("valgrind -q --tool=massif --massif-out-file={} {}{}",
                        output,
                        measure_pages_opt,
                        max_details_opt)
            } else {
                "".into()
            };

            format!("{}{} {}",
                    valgrind_cmd,
                    command.unexpanded_visible,
                    command.unexpanded_hidden)
        }

        let output_massif = &(output.to_string() + ".massif");

        let script = &bash_in_out_script(&input.expanded,
                                         output,
                                         &build_cmd(profile.with_mem,
                                                    output_massif,
                                                    command));

        let time_start = time::precise_time_ns();
        let out = bash_run(script);

        let cmd_out: &str = &String::from_utf8_lossy(&out.stdout);
        let cmd_err: &str = &String::from_utf8_lossy(&out.stderr);

        let time_end = time::precise_time_ns();

        let time_span = ((time_end - time_start) as f64)
            / 1000.0
            / 1000.0
            / 1000.0;
        let inp_size = &nice_size(input.size);

        if out.status.success() {
            let out_size = fs::metadata(output).unwrap().len();
            let comp_size = &nice_size(out_size);
            let ratio = (out_size as f64) / (input.size as f64) * 100.0;

            let mem_peak = if profile.with_mem {
                use std::fs::File;
                use std::io::BufReader;
                use std::io::BufRead;

                let report_file = File::open(output_massif).unwrap();
                let report_file = BufReader::new(report_file);

                let peak = report_file.lines().filter_map(|line| {
                    let line = line.unwrap();
                    let p = "mem_heap_B=";
                    if line.starts_with(p) {
                        Some(line[p.len()..].to_owned())
                    } else {
                        None
                    }
                })
                .map(|size| size.parse::<u64>().unwrap())
                .max().unwrap();

                let peak = nice_size(peak);
                Some(peak)
            } else {
                None
            };
            let mem_peak = mem_peak.as_ref().map(|s| &**s);

            print_line(
                (kind, label, time_span, inp_size, comp_size, ratio, mem_peak),
                padding
            );
            if !cmd_out.is_empty() {
                print_sep(padding);
                print!("{}", cmd_out);
                print_sep(padding);
            }
        } else {
            print_line(
                ("ERR", label, time_span, inp_size, "", 0.0/0.0, None),
                padding
            );
            let a = !cmd_out.is_empty();
            let b = !cmd_err.is_empty();
            if a {
                print_sep(padding);
                print!("{}", cmd_out);
            }
            if a || b {
                print_sep(padding);
            }
            if b {
                print!("{}", cmd_err);
                print_sep(padding);
            }
        }
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
type Line<'a> = (&'a str, &'a str, f64, &'a str, &'a str, f64, Option<&'a str>);
type LinePad = (usize,);

fn print_header(kind: &str, value: &str, padding: LinePad) {
    println!("{:4} | {:7$} | {:^9} | {:^12} | {:^12} | {:^8} | {:^12}",
        kind, value, "time", "size", "comp. size", "ratio", "mem", padding.0);
}

fn print_line(line: Line, padding: LinePad) {
    let mem = line.6.unwrap_or("-");
    println!("{:4} | {:7$} | {:7.3} s | {:>12} | {:>12} | {:6.2} % | {:^12}",
        line.0, line.1, line.2, line.3, line.4, line.5, mem, padding.0);
}

fn print_sep(padding: LinePad) {
    println!("{:-^4}---{:-^7$}---{:-^7}-----{:-^12}---{:-^12}---{:-^6}---{:-^12}--",
        "", "", "", "", "", "", "", padding.0);
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

fn _alphabet_size(file: &str) -> usize {
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
