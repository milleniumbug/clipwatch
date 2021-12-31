#[cfg_attr(unix, path = "linux.rs")]
#[cfg_attr(windows, path = "win.rs")]
mod clipwatch;

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        let result = 2 + 2;
        assert_eq!(result, 4);
    }
}
