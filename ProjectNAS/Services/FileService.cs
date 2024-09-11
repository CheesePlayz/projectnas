using ProjectNAS.Models;

namespace ProjectNAS.Services
{
    public class FileService
    {
        public List<FileModel> UploadedFiles { get; private set; }

        public FileService()
        {
            UploadedFiles = new List<FileModel>();
        }

        public List<FileModel> GetFiles()
        {
            UploadedFiles.Clear();

            var folderPath = Path.Combine(Directory.GetCurrentDirectory(), "Uploads");

            if (!Directory.Exists(folderPath))
            {
                Directory.CreateDirectory(folderPath);
            }

            var fileList = Directory.GetFiles(folderPath);

          
            if (fileList != null)
            {
                foreach (var file in fileList)
                {
                    var fileInfo = new FileInfo(file); // Get file info for each file

                    var fileModel = new FileModel
                    {
                        FileName = fileInfo.Name, // Get file name
                        LastModified = fileInfo.LastWriteTime.ToString("o"), // Get last modified date in ISO 8601 format
                        ContentType = fileInfo.Extension, // Get MIME type based on file extension
                        Size = fileInfo.Length // Get file size in bytes
                    };

                    UploadedFiles.Add(fileModel); // Add the file model to the list
                }
                return UploadedFiles;
            }
            return null;
        }

        public string[] GetDirectories()
        {
            var folderPath = Path.Combine(Directory.GetCurrentDirectory(), "Uploads");

            string[] directories = Directory.GetDirectories(folderPath);
            return directories;
        }

    }
}
