namespace ProjectNAS.Models
{
    public class FileModel
    {
        public string? FileName { get; set; }
        public string? LastModified { get; set; }
        public string? ContentType { get; set; }
        public long? Size { get; set; }
    }
}
