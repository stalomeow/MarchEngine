namespace March.Core.Diagnostics
{
    // https://stackoverflow.com/questions/2031163/when-to-use-the-different-log-levels

    public enum LogLevel
    {
        /// <summary>
        /// Only when I would be "tracing" the code and trying to find one part of a function specifically.
        /// </summary>
        Trace,

        /// <summary>
        /// Information that is diagnostically helpful to people more than just developers (IT, sysadmins, etc.).
        /// </summary>
        Debug,

        /// <summary>
        /// Generally useful information to log (service start/stop, configuration assumptions, etc).
        /// Info I want to always have available but usually don't care about under normal circumstances.
        /// This is my out-of-the-box config level.
        /// </summary>
        Info,

        /// <summary>
        /// Anything that can potentially cause application oddities, but for which I am automatically recovering.
        /// (Such as switching from a primary to backup server, retrying an operation, missing secondary data, etc.)
        /// </summary>
        Warning,

        /// <summary>
        /// Any error which is fatal to the operation, but not the service or application (can't open a required file, missing data, etc.).
        /// These errors will force user (administrator, or direct user) intervention.
        /// These are usually reserved (in my apps) for incorrect connection strings, missing services, etc.
        /// </summary>
        Error,
    }
}
