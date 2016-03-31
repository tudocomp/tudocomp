use toml;
use std::collections::BTreeMap;

#[derive(Debug)]
pub struct Profile {
    pub inputs: Vec<String>,
    pub commands: Vec<(String, String)>,
    pub compare_commands: bool,
    pub with_mem: bool,
    pub runtime_iterations: Option<i64>,
}

#[derive(Debug)]
pub struct Config {
    pub profiles: BTreeMap<String, Profile>,
}

#[derive(Debug)]
pub struct TomlErr;

fn parse_profile(name: &str, toml: &toml::Table) -> Result<Profile, TomlErr> {
    let mut profile = Profile {
        commands: vec![],
        inputs: vec![],
        compare_commands: true,
        with_mem: false,
        runtime_iterations: None,
    };

    let toml_entry = try! {
        toml.get(name).and_then(|entry| entry.as_table()).ok_or(TomlErr)
    };

    if let Some(parent) = toml_entry.get("inherit_from") {
        let parent = try!(parent.as_str().ok_or(TomlErr));
        profile = try!(parse_profile(parent, toml));
    }

    if let Some(commands) = toml_entry.get("commands") {
        let commands = try!(commands.as_slice().ok_or(TomlErr));
        profile.commands.clear();
        for command in commands {
            let command = try!(command.as_slice().ok_or(TomlErr));
            let cmd0 = try!(command.get(0).and_then(|s| s.as_str()).ok_or(TomlErr)).to_owned();
            let cmd1 = try!(command.get(1).and_then(|s| s.as_str()).ok_or(TomlErr)).to_owned();

            profile.commands.push((cmd0, cmd1));
        }
    }

    if let Some(inputs) = toml_entry.get("inputs") {
        let inputs = try!(inputs.as_slice().ok_or(TomlErr));
        profile.inputs.clear();
        for input in inputs {
            let input = try!(input.as_str().ok_or(TomlErr)).to_owned();

            profile.inputs.push(input);
        }
    }

    if let Some(compare_commands) = toml_entry.get("compare_commands") {
        profile.compare_commands = try! {
            compare_commands.as_bool().ok_or(TomlErr)
        };
    }

    if let Some(with_mem) = toml_entry.get("with_mem") {
        profile.with_mem = try! {
            with_mem.as_bool().ok_or(TomlErr)
        };
    }

    if let Some(runtime_iterations) = toml_entry.get("iterations") {
        profile.runtime_iterations = Some(try! {
            runtime_iterations.as_integer().ok_or(TomlErr)
        });
    }

    Ok(profile)
}

pub fn parse_config(toml: toml::Table) -> Result<Config, TomlErr> {
    let mut config = Config { profiles: BTreeMap::new() };
    for (key, _) in &toml {
        config.profiles.insert(key.to_owned(), try!(parse_profile(key, &toml)));
    }
    Ok(config)
}
