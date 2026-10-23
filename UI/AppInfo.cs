namespace Mesen
{
	/// <summary>
	/// Central place for the fork's branding (name, links, platform identifiers).
	/// The internal "Mesen" namespaces and the MesenCore native library keep their
	/// upstream names on purpose, to keep merging changes from MesenCE simple.
	/// </summary>
	public static class AppInfo
	{
		public const string Name = "RetroStationX";
		public const string Description = "Multi-system emulator";
		public const string CopyrightHolder = "ksdevla";

		//RetroStationX's own version (set by <Version> in UI.csproj), e.g 2.2.1.1 = based on MesenCE 2.2.1, fork revision 1
		//The core keeps the upstream version (EmuApi.GetMesenVersion) - save states, movies and netplay use it for compatibility checks
		public static readonly System.Version Version = typeof(AppInfo).Assembly.GetName().Version ?? new System.Version(0, 0, 0, 0);

		public const string RepoUrl = "https://github.com/ksdevla/RetroStationX";
		public const string IssuesUrl = RepoUrl + "/issues";
		public const string CommitUrl = RepoUrl + "/commit/";

		//URL of the JSON update feed (same format as MesenCE's latestversion.json).
		//Leave empty to disable the built-in updater until the fork publishes its own feed.
		public const string UpdateFeedUrl = "";

		//Download URLs listed in the update feed must start with this prefix
		public const string UpdateDownloadUrlPrefix = "https://github.com/ksdevla/";

		//Identifiers used for OS integration - these must not match MesenCE's
		//so both emulators can be installed side by side
		public const string FileAssociationProgId = "RetroStationX";
		public const string LinuxMimeTypePrefix = "x-retrostationx-";
		public const string LinuxIconName = "RetroStationX";
		public const string LinuxDesktopFileName = "retrostationx.desktop";
	}
}
