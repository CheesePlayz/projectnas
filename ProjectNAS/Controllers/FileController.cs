using Microsoft.AspNetCore.Mvc;
using ProjectNAS.Models;
using ProjectNAS.Services;
using System.IO.Compression;
using System.Runtime.CompilerServices;

namespace ProjectNAS.Controllers
{
    public class FileController : Controller
    {
        private readonly FileService _fileService;
        private string? uploadPath;

        public FileController(FileService fileService)
        {
            _fileService = fileService;
            uploadPath = Path.Combine(Directory.GetCurrentDirectory(), "Uploads");
        }

        [HttpPost]
        public async Task<IActionResult> Upload(List<IFormFile> files)
        {

            if (files == null || files.Count == 0)
            {
                return BadRequest("No files received.");
            }

            if (!Directory.Exists(uploadPath))
            {
                Directory.CreateDirectory(uploadPath);
            }

            var fileDataList = new List<FileModel>();

            foreach (var file in files)
            {
                if (file.Length > 0)
                {
                    var filePath = Path.Combine(uploadPath, file.FileName);
                    using (var stream = new FileStream(filePath, FileMode.Create))
                    {
                        await file.CopyToAsync(stream);
                    }
                }
            }
            return RedirectToAction("Index", "Home");
        }

        [HttpGet]
        public async Task<IActionResult> Download(string fileName)
        {
            var filePath = Path.Combine(uploadPath, fileName);
            if (!System.IO.File.Exists(filePath))
            {
                return NotFound("File not found.");
            }

            var memory = new MemoryStream();
            using (var stream = new FileStream(filePath, FileMode.Open))
            {
                stream.CopyTo(memory);
            }
            memory.Position = 0;

            return File(memory, "application/octet-stream", fileName);
        }

        public async Task<IActionResult> ZipDownload(string fileName)
        {
            var filePath = Path.Combine(uploadPath, fileName);
            if (!System.IO.File.Exists(filePath))
            {
                return NotFound("File not found.");
            }

            string zipFilePath = Path.ChangeExtension(filePath, ".zip");
            try
            {
                using (ZipArchive archive = ZipFile.Open(zipFilePath, ZipArchiveMode.Create))
                {
                    archive.CreateEntryFromFile(filePath, Path.GetFileName(filePath));
                }

                var memory = new MemoryStream();

                using (var stream = new FileStream(zipFilePath, FileMode.Open, FileAccess.Read))
                {
                    await stream.CopyToAsync(memory);
                }
                memory.Position = 0;
                return File(memory, "application/octet-stream", Path.GetFileName(zipFilePath));
            }
            finally
            {
                if (System.IO.File.Exists(zipFilePath))
                {
                    System.IO.File.Delete(zipFilePath);
                }
            }
        }
    }
}
