// SynchronizeDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "编程助理.h"
#include "SynchronizeDlg.h"
#include "afxdialogex.h"

#include "TransmissionDlg.h"
#include "WebProjectDlg.h"

// CSynchronizeDlg 对话框

IMPLEMENT_DYNAMIC(CSynchronizeDlg, CDialogEx)

CSynchronizeDlg::CSynchronizeDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSynchronizeDlg::IDD, pParent)
{
	IsCode = TRUE;
	m_hOperate = NULL;
	Type = 1;
}

CSynchronizeDlg::~CSynchronizeDlg()
{
}

void CSynchronizeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOCAL_TREE, m_Local);
	DDX_Control(pDX, IDC_SERVER_TREE, m_Server);
}


BEGIN_MESSAGE_MAP(CSynchronizeDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSynchronizeDlg::OnOK)
	ON_BN_CLICKED(IDCANCEL, &CSynchronizeDlg::OnCancel)
	ON_NOTIFY(NM_CLICK, IDC_LOCAL_TREE, &CSynchronizeDlg::OnClickLocalTree)
	ON_NOTIFY(NM_CLICK, IDC_SERVER_TREE, &CSynchronizeDlg::OnClickServerTree)
	ON_NOTIFY(NM_DBLCLK, IDC_LOCAL_TREE, &CSynchronizeDlg::OnDblclkLocalTree)
	ON_NOTIFY(NM_DBLCLK, IDC_SERVER_TREE, &CSynchronizeDlg::OnDblclkServerTree)
	ON_BN_CLICKED(IDC_UPLOAD_BUTTON, &CSynchronizeDlg::OnUpload)
	ON_BN_CLICKED(IDC_DOWNLOAD_BUTTON, &CSynchronizeDlg::OnDownload)
	ON_BN_CLICKED(IDC_SWITCH_BUTTON, &CSynchronizeDlg::OnSwitch)
	ON_BN_CLICKED(IDC_VERSION_BUTTON, &CSynchronizeDlg::OnVersion)

	ON_COMMAND(100, &CSynchronizeDlg::OnSuccess)
	ON_COMMAND(101, &CSynchronizeDlg::OnError)
END_MESSAGE_MAP()


// CSynchronizeDlg 消息处理程序


BOOL CSynchronizeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 刷新数据
	HTREEITEM m_TreeRoot = m_Local.InsertItem(_T("本地源码库"));//插入根节点
	ShowFile(_T("Code"), m_TreeRoot);//根目录进行遍历


	// 获取服务器数据
	GetServerInfo();

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


BOOL CSynchronizeDlg::PreTranslateMessage(MSG* pMsg)
{
	return CDialogEx::PreTranslateMessage(pMsg);
}


void CSynchronizeDlg::ShowFile(CString str_Dir, HTREEITEM tree_Root)
{
    CFileFind FileFind;

    //临时变量，用以记录返回的树节点
    HTREEITEM tree_Temp;

    //判断输入目录最后是否存在'\'，不存在则补充
    if (str_Dir.Right(1) != "\\")
        str_Dir += "\\";
    str_Dir += "*.*";
    BOOL res = FileFind.FindFile(str_Dir);
    while (res)
    {
        tree_Temp = tree_Root;
        res = FileFind.FindNextFile();
        if (FileFind.IsDirectory() && !FileFind.IsDots())//目录是文件夹
        {
            CString strPath = FileFind.GetFilePath(); //得到路径，做为递归调用的开始
            CString strTitle = FileFind.GetFileName();//得到目录名，做为树控的结点
            tree_Temp = m_Local.InsertItem(strTitle, 0, 0, tree_Root);
            ShowFile(strPath, tree_Temp);
        }
        else if (!FileFind.IsDirectory() && !FileFind.IsDots())//如果是文件
        {
            CString strPath = FileFind.GetFilePath(); //得到路径，做为递归调用的开始
            CString strTitle = FileFind.GetFileName();//得到文件名，做为树控的结点
            m_Local.InsertItem(strTitle, 0, 0, tree_Temp);
        }
    }

    FileFind.Close();
}


void CSynchronizeDlg::ConsistentParentCheck(CTreeCtrl * pTreeCtrl, HTREEITEM hTreeItem)
{
	// 获取父节点，若为空则返回，否则处理状态
	HTREEITEM hParentItem = pTreeCtrl->GetParentItem(hTreeItem);
	if(hParentItem != NULL)
	{
		// 依次判断选中项父节点各子节点状态
		HTREEITEM hChildItem = pTreeCtrl->GetChildItem(hParentItem);
		while(hChildItem != NULL)
		{
			// 若有一个子节点未选中，则父节点也未选中，同时递归处理父节点的父节点
			if(pTreeCtrl->GetCheck(hChildItem) == FALSE)
			{
				pTreeCtrl->SetCheck(hParentItem, FALSE);
				return ConsistentParentCheck(pTreeCtrl, hParentItem);
			}

			// 获取父节点的下一个子节点
			hChildItem = pTreeCtrl->GetNextItem(hChildItem, TVGN_NEXT);
		}

		// 若子节点全部选中，则父节点也选中，同时递归处理父节点的父节点
		pTreeCtrl->SetCheck(hParentItem, TRUE);
		return ConsistentParentCheck(pTreeCtrl, hParentItem);
	}
}


void CSynchronizeDlg::ConsistentChildCheck(CTreeCtrl  * pTreeCtrl, HTREEITEM hTreeItem)
{
	// 获取当前复选框选中状态
	BOOL bCheck = pTreeCtrl->GetCheck(hTreeItem);

	// 若当前节点存在子节点，则一致化子节点状态
	if(pTreeCtrl->ItemHasChildren(hTreeItem))
	{
		// 依次一致化子节点的子节点状态
		HTREEITEM hChildItem = pTreeCtrl->GetChildItem(hTreeItem);
		while(hChildItem != NULL)
		{
			pTreeCtrl->SetCheck(hChildItem, bCheck);
			ConsistentChildCheck(pTreeCtrl, hChildItem);

			hChildItem = pTreeCtrl->GetNextItem(hChildItem, TVGN_NEXT);
		}
	}
}


void CSynchronizeDlg::Split(CString source, CString divKey, CStringArray &dest)
{
	dest.RemoveAll();
	int pos = 0;
	int pre_pos = 0;
	while ( -1 != pos )
	{
		pre_pos = pos;
		pos     = source.Find(divKey, (pos +1));

		CString temp(source.Mid(pre_pos , (pos -pre_pos )));
		temp.Replace(divKey, _T(""));
		dest.Add(temp);
	}
}


void CSynchronizeDlg::GetServerInfo()
{
	// 启动工作者线程
	if (m_hOperate == NULL)
	{
		Parameter.Format(_T("id=%s&type=%d"), UserId, Type);
		m_hOperate = AfxBeginThread(Operate, this);
	}
}


void CSynchronizeDlg::OnSuccess()
{
	switch(Type)
	{
	case 1:
		{
			CStringArray TextArray;
			Split(ServerInfo, _T(";"), TextArray);

			HTREEITEM rootItem = m_Server.InsertItem(_T("云端源码库"), 0, 0, NULL), parentItem, subItem;

			CString Parent, Sub;
			for(int i=0; i<TextArray.GetSize() -1; i++)
			{
				CString FileInfo = TextArray.GetAt(i);

				// 数据处理
				FileInfo.Replace(_T("cloud/") + UserId + _T("/Code/"), _T(""));
				FileInfo.Replace(_T("cloud/") + UserId + _T("/Code"), _T(""));

				if (FileInfo.Replace(_T(".."), _T("..")) || FileInfo.Replace(_T("/."), _T("/.")) || FileInfo == _T(".") || FileInfo == _T(""))
				{
					//目录
					continue;
				}
				else if(FileInfo.Replace(_T("/"), _T("/")) && !FileInfo.Replace(_T("."), _T(".")))
				{
					// 子文件夹
					FileInfo.Replace(Parent + _T("/"), _T(""));
					subItem = m_Server.InsertItem(FileInfo, 2, 2, parentItem);
					Sub = Parent + _T("/") + FileInfo;
				}
				else if(FileInfo.Replace(_T("."), _T(".")))
				{
					// 文件
					FileInfo.Replace(Sub + _T("/"), _T(""));
					m_Server.InsertItem(FileInfo, 3, 3, subItem);
				}
				else 
				{
					// 文件夹
					Parent = FileInfo;
					parentItem = m_Server.InsertItem(FileInfo, 1, 1, rootItem);
				}
			}
		} break;

	case 2:
		{
			CStringArray TextArray;
			Split(ServerInfo, _T(";"), TextArray);

			HTREEITEM rootItem = m_Server.InsertItem(_T("云端文件库"), 0, 0, NULL), parentItem, subItem, subFile;

			CString Parent, Sub, Item, Function;
			for(int i=0; i<TextArray.GetSize() -1; i++)
			{
				CString FileInfo = TextArray.GetAt(i);

				// 数据处理
				FileInfo.Replace(_T("cloud/") + UserId + _T("/File/"), _T(""));
				FileInfo.Replace(_T("cloud/") + UserId + _T("/File"), _T(""));

				if (FileInfo.Replace(_T(".."), _T("..")) || FileInfo.Replace(_T("/."), _T("/.")) || FileInfo == _T(".") || FileInfo == _T(""))
				{
					//目录
					continue;
				}

				else if(FileInfo.Replace(_T("/"), _T("/")) && !FileInfo.Replace(_T("."), _T(".")))
				{
					// 方法
					if(!Item.IsEmpty() && FileInfo.Replace(Item + _T("/"), Item + _T("/")))
					{
						FileInfo.Replace(Sub + _T("/"), _T(""));
						subFile = m_Server.InsertItem(FileInfo, 3, 3, subItem);
						Function= FileInfo;
					}
					else
					{
						// 子文件夹
						FileInfo.Replace(Parent + _T("/"), _T(""));
						subItem = m_Server.InsertItem(FileInfo, 2, 2, parentItem);
						Sub = Parent + _T("/") + FileInfo;
						Item= FileInfo;
					}
				}

				else if(FileInfo.Replace(_T("."), _T(".")))
				{
					// 文件
					FileInfo.Replace(Sub + _T("/") + Function + _T("/"), _T(""));
					m_Server.InsertItem(FileInfo, 4, 4, subFile);
				}

				else 
				{
					// 文件夹
					Parent = FileInfo;
					parentItem = m_Server.InsertItem(FileInfo, 1, 1, rootItem);
				}
			}
		} break;
	}

	
}


void CSynchronizeDlg::OnError()
{
	AfxMessageBox(Error);
}


void CSynchronizeDlg::GetTreeData(CTreeCtrl * pTreeCtrl, HTREEITEM hitem, BOOL IsCheck)
{
	if(hitem != NULL)
	{
		hitem = pTreeCtrl->GetChildItem(hitem);
		while(hitem)
		{
			if(IsCheck)
			{
				// 如果选中
				if(pTreeCtrl->GetCheck(hitem))
				{
					CString text = pTreeCtrl->GetItemText(hitem);

					// 是文件
					if(text.Replace(_T("."), _T(".")))
					{
						IsFile = true;

						if(IsCode)
							TargetList.Add( UserId + _T("/code") + TreeData + _T("/") + text);
						else
							TargetList.Add( UserId + _T("/file") + TreeData + _T("/") + text);
					}
					else
					{
						IsFile = false;
						TreeData += _T("/") + text;
					}
				}
			}
			else
			{
				CString text = pTreeCtrl->GetItemText(hitem);

				// 是文件
				if(text.Replace(_T("."), _T(".")))
				{
					IsFile = true;

					if(IsCode)
						TargetList.Add( UserId + _T("/code") + TreeData + _T("/") + text);
					else
						TargetList.Add( UserId + _T("/file") + TreeData + _T("/") + text);
				}
				else
				{
					IsFile = false;
					TreeData += _T("/") + text;
				}
			}

			// 递归
			GetTreeData(pTreeCtrl, hitem, IsCheck);
			hitem = pTreeCtrl->GetNextItem(hitem, TVGN_NEXT);

			// 清除多余路径
			if(!IsFile)
			{
				TreeData = _T("");
			}
			else
			{
				IsFile = false;
			}
		}
	}
}


// 工作线程
UINT CSynchronizeDlg::Operate(LPVOID pParam)
{
	// 窗口指针
	CSynchronizeDlg * pWnd = ((CSynchronizeDlg*)pParam);

	// 局部变量
	CString RecvData;
	BOOL IsSuccess;

	// 禁用按钮
	pWnd->GetDlgItem(IDC_UPLOAD_BUTTON)->EnableWindow(FALSE);
	pWnd->GetDlgItem(IDOK)->EnableWindow(FALSE);
	pWnd->GetDlgItem(IDC_DOWNLOAD_BUTTON)->EnableWindow(FALSE);
	pWnd->GetDlgItem(IDC_SWITCH_BUTTON)->EnableWindow(FALSE);
	pWnd->GetDlgItem(IDC_VERSION_BUTTON)->EnableWindow(FALSE);

	// 获取服务器数据
	try
	{
		RecvData = theApp.OnGetWebInfo(_T("www.shadowviolet.cn"), _T("index/account/GetFileInfo"), 80, pWnd->Parameter, IsSuccess);
		if (RecvData == _T("") || RecvData.IsEmpty() || !IsSuccess)
		{
			pWnd->Error = _T("无法连接到服务器, 请检查网络。");
			pWnd->PostMessage(WM_COMMAND, 101);
		}
		else
		{
			if (IsSuccess)
			{
				if( RecvData.Replace(_T(";"), _T(";")) )
				{
					pWnd->ServerInfo = RecvData;
					pWnd->PostMessage(WM_COMMAND, 100);
				}
				else
				{
					pWnd->Error = _T("数据读取失败，请稍后再试。");
					pWnd->PostMessage(WM_COMMAND, 101);
				}
			}
			else
			{
				pWnd->Error = _T("无法连接到服务器, 请检查网络。");
				pWnd->PostMessage(WM_COMMAND, 101);
			}
		}
	}
	catch (...)
	{
		pWnd->Error = _T("发生了异常，位于Operate的OnGetWebInfo方法。");
		pWnd->PostMessage(WM_COMMAND, 101);
	}

	// 激活按钮
	pWnd->GetDlgItem(IDC_UPLOAD_BUTTON)->EnableWindow();
	pWnd->GetDlgItem(IDOK)->EnableWindow();
	pWnd->GetDlgItem(IDC_DOWNLOAD_BUTTON)->EnableWindow();
	pWnd->GetDlgItem(IDC_SWITCH_BUTTON)->EnableWindow();
	pWnd->GetDlgItem(IDC_VERSION_BUTTON)->EnableWindow();

	// 对象置为空
	pWnd->m_hOperate = NULL;
	return false;
}


void CSynchronizeDlg::OnClickLocalTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	CPoint oPoint;
	UINT   nFlag;
	GetCursorPos(&oPoint);
	m_Local.ScreenToClient(&oPoint);

	HTREEITEM oSelectItem = m_Local.HitTest(oPoint, &nFlag);
	if(oSelectItem == NULL)
	{
		return;
	}

	m_Local.SelectItem(oSelectItem);
	if(nFlag & TVHT_ONITEMSTATEICON)
	{
		BOOL bCheck = !m_Local.GetCheck(oSelectItem);

		// 为了一致化选中状态，需设置复选框为改变后的状态
		m_Local.SetCheck(oSelectItem, bCheck);

		// 一致化选中状态
		ConsistentParentCheck(&m_Local, oSelectItem);
		ConsistentChildCheck(&m_Local,oSelectItem);

		// 复选框状态复原，MFC自动响应改变绘制
		m_Local.SetCheck(oSelectItem, !bCheck);
	}

	*pResult = 0;
}


void CSynchronizeDlg::OnDblclkLocalTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnUpload();
	*pResult = 0;
}


void CSynchronizeDlg::OnClickServerTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	CPoint oPoint;
	UINT   nFlag;
	GetCursorPos(&oPoint);
	m_Server.ScreenToClient(&oPoint);

	HTREEITEM oSelectItem = m_Server.HitTest(oPoint, &nFlag);
	if(oSelectItem == NULL)
	{
		return;
	}

	m_Server.SelectItem(oSelectItem);
	if(nFlag & TVHT_ONITEMSTATEICON)
	{
		BOOL bCheck = !m_Server.GetCheck(oSelectItem);

		// 为了一致化选中状态，需设置复选框为改变后的状态
		m_Server.SetCheck(oSelectItem, bCheck);

		// 一致化选中状态
		ConsistentParentCheck(&m_Server, oSelectItem);
		ConsistentChildCheck(&m_Server, oSelectItem);

		// 复选框状态复原，MFC自动响应改变绘制
		m_Server.SetCheck(oSelectItem, !bCheck);
	}
	*pResult = 0;
}


void CSynchronizeDlg::OnDblclkServerTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnDownload();
	*pResult = 0;
}


void CSynchronizeDlg::OnOK()
{
	// 初始化数据
	IsFile = false;
	TreeData = _T("");
	TargetList.RemoveAll();

	// 树数据列表
	CStringArray Local_TargetList, Server_TargetList;

	// 得到本地树数据
	HTREEITEM root = m_Local.GetRootItem();
	GetTreeData(&m_Local, root, false);
	Local_TargetList.Append(TargetList);

	// 初始化数据
	IsFile = false;
	TreeData = _T("");
	TargetList.RemoveAll();

	// 得到云端树数据
	root = m_Server.GetRootItem();
	GetTreeData(&m_Server, root, false);
	Server_TargetList.Append(TargetList);

	// 对比云端与本地数据数量
	// 云端没有本地数据
	if(Server_TargetList.GetSize() < Local_TargetList.GetSize())
	{
		// 得到云端缺失的本地数据列表
		for(int i=0; i<Server_TargetList.GetSize(); i++)
		{
			CString LocalData  = Local_TargetList .GetAt(i);
			CString ServerData = Server_TargetList.GetAt(i);

			// 除去云端已有数据
			if(LocalData == ServerData)
			{
				Local_TargetList.RemoveAt(i);
			}
		}

		// 需要上传数据
		if(Local_TargetList.GetSize() > 0)
		{
			CTransmissionDlg dlg;
			dlg.TargetList = &Local_TargetList;
			dlg.IsDownload = false;
			dlg.IsCode     = IsCode;
			dlg.DoModal();

			if(dlg.IsFinished)
			{
				// 清空云端列表
				m_Server.DeleteAllItems();

				// 类型判断
				if(IsCode)
				{
					Type = 1;
				}
				else
				{
					Type = 2;
				}

				// 刷新云端数据
				GetServerInfo();
			}
		}
		else
		{
			AfxMessageBox(_T("没有需要同步的文件!"));
		}
	}

	// 本地没有云端数据
	else if(Server_TargetList.GetSize() > Local_TargetList.GetSize())
	{
		// 得到本地缺失的云端数据列表
		for(int i=0; i<Local_TargetList.GetSize(); i++)
		{
			CString LocalData  = Local_TargetList .GetAt(i);
			CString ServerData = Server_TargetList.GetAt(i);

			// 除去本地已有数据
			if(LocalData == ServerData)
			{
				Server_TargetList.RemoveAt(i);
			}
		}

		// 需要下载云端数据
		if(Server_TargetList.GetSize() > 0)
		{
			CTransmissionDlg dlg;
			dlg.TargetList = &Server_TargetList;
			dlg.IsDownload = true;
			dlg.IsCode     = IsCode;
			dlg.DoModal();

			if(dlg.IsFinished)
			{
				// 清空本地列表
				m_Local.DeleteAllItems();

				// 判断类型
				if(IsCode)
				{
					// 刷新本地数据
					HTREEITEM m_TreeRoot = m_Local.InsertItem(_T("本地源码库"));//插入根节点
					ShowFile(_T("Code"), m_TreeRoot);//根目录进行遍历
				}
				else
				{
					// 刷新本地数据
					HTREEITEM m_TreeRoot = m_Local.InsertItem(_T("本地文件库"));//插入根节点
					ShowFile(_T("File"), m_TreeRoot);//根目录进行遍历
				}
			}
		}
		else
		{
			AfxMessageBox(_T("没有需要同步的文件!"));
		}
	}

	// 判断云端与本地数据的修改时间
	else
	{
		CStringArray Local_Time, Server_Time, UpLoad, DownLoad;

		// 得到本地文件修改时间
		for(int i=0; i<Local_TargetList.GetSize(); i++)
		{
			// 处理数据
			CString LocalPath = Local_TargetList.GetAt(i);
			LocalPath = LocalPath.Right(LocalPath.GetLength() - LocalPath.Find('/') -1);

			CFileStatus Status;
			if(CFile::GetStatus(LocalPath, Status))
			{
				CString ModifyTime = Status.m_mtime.Format("%Y-%m-%d %H:%M:%S");
				Local_Time.Add(ModifyTime);
			}
		}

		for(int i=0; i<Server_TargetList.GetSize(); i++)
		{
			// 获取云端文件修改时间
			try
			{
				// 局部变量
				CString RecvData;
				BOOL IsSuccess;
				Parameter.Format(_T("path=%s"), Server_TargetList.GetAt(i));

				RecvData = theApp.OnGetWebInfo(_T("www.shadowviolet.cn"), _T("index/account/GetFileModifyTime"), 80, Parameter, IsSuccess);
				if (RecvData == _T("") || RecvData.IsEmpty() || !IsSuccess)
				{
					AfxMessageBox(_T("无法连接到服务器, 请检查网络。"));
				}
				else
				{
					if (IsSuccess)
					{
						if( RecvData.Replace(_T(":"), _T(":")) )
						{
							// 添加云端文件修改时间
							Server_Time.Add(RecvData);
						}
						else
						{
							AfxMessageBox(_T("数据读取失败，请稍后再试。"));
						}
					}
					else
					{
						AfxMessageBox(_T("无法连接到服务器, 请检查网络。"));
					}
				}
			}
			catch (...)
			{
				AfxMessageBox(_T("发生了异常，位于OK的OnGetWebInfo方法。"));
			}
		}

		// 比对文件修改时间
		for(int i=0; i<Local_Time.GetSize(); i++)
		{
			CString LocalData  = Local_Time .GetAt(i);
			CString ServerData = Server_Time.GetAt(i);

			// 如果数据不同
			if(LocalData != ServerData)
			{
				if(LocalData > ServerData)
				{
					UpLoad.Add(Local_TargetList.GetAt(i));
				}
				else
				{
					DownLoad.Add(Server_TargetList.GetAt(i));
				}
			}
		}

		// 处理传输队列
		if(UpLoad.GetSize() > 0)
		{
			CTransmissionDlg dlg;
			dlg.TargetList = &UpLoad;
			dlg.IsDownload = false;
			dlg.IsCode     = IsCode;
			dlg.DoModal();

			if(dlg.IsFinished)
			{
				// 清空云端列表
				m_Server.DeleteAllItems();

				// 类型判断
				if(IsCode)
				{
					Type = 1;
				}
				else
				{
					Type = 2;
				}

				// 刷新云端数据
				GetServerInfo();
			}

			MessageBox(_T("云端已与本地数据完全同步!"));
		}
		else if(DownLoad.GetSize() > 0)
		{
			CTransmissionDlg dlg;
			dlg.TargetList = &DownLoad;
			dlg.IsDownload = true;
			dlg.IsCode     = IsCode;
			dlg.DoModal();

			if(dlg.IsFinished)
			{
				// 清空本地列表
				m_Local.DeleteAllItems();

				// 判断类型
				if(IsCode)
				{
					// 刷新本地数据
					HTREEITEM m_TreeRoot = m_Local.InsertItem(_T("本地源码库"));//插入根节点
					ShowFile(_T("Code"), m_TreeRoot);//根目录进行遍历
				}
				else
				{
					// 刷新本地数据
					HTREEITEM m_TreeRoot = m_Local.InsertItem(_T("本地文件库"));//插入根节点
					ShowFile(_T("File"), m_TreeRoot);//根目录进行遍历
				}

				MessageBox(_T("云端已与本地数据完全同步!"));
			}
		}
		else
		{
			MessageBox(_T("云端已与本地数据完全同步!"));
		}
	}
}


void CSynchronizeDlg::OnUpload()
{
	// 初始化数据
	IsFile = false;
	TreeData = _T("");
	TargetList.RemoveAll();

	// 得到树数据
	HTREEITEM root = m_Local.GetRootItem();
	GetTreeData(&m_Local, root, true);

	if(TargetList.GetSize() > 0)
	{
		CTransmissionDlg dlg;
		dlg.TargetList = &TargetList;
		dlg.IsDownload = false;
		dlg.IsCode     = IsCode;
		dlg.DoModal();

		if(dlg.IsFinished)
		{
			// 清空云端列表
			m_Server.DeleteAllItems();

			// 类型判断
			if(IsCode)
			{
				Type = 1;
			}
			else
			{
				Type = 2;
			}

			// 刷新云端数据
			GetServerInfo();
		}
	}
	else
	{
		AfxMessageBox(_T("请勾选需要上传的文件!"));
	}
}


void CSynchronizeDlg::OnDownload()
{
	// 初始化数据
	IsFile = false;
	TreeData = _T("");
	TargetList.RemoveAll();

	// 得到树数据
	HTREEITEM root = m_Server.GetRootItem();
	GetTreeData(&m_Server, root, true);

	if(TargetList.GetSize() > 0)
	{
		CTransmissionDlg dlg;
		dlg.TargetList = &TargetList;
		dlg.IsDownload = true;
		dlg.IsCode     = IsCode;
		dlg.DoModal();

		if(dlg.IsFinished)
		{
			// 清空本地列表
			m_Local.DeleteAllItems();

			// 判断类型
			if(IsCode)
			{
				// 刷新本地数据
				HTREEITEM m_TreeRoot = m_Local.InsertItem(_T("本地源码库"));//插入根节点
				ShowFile(_T("Code"), m_TreeRoot);//根目录进行遍历
			}
			else
			{
				// 刷新本地数据
				HTREEITEM m_TreeRoot = m_Local.InsertItem(_T("本地文件库"));//插入根节点
				ShowFile(_T("File"), m_TreeRoot);//根目录进行遍历
			}
		}
	}
	else
	{
		AfxMessageBox(_T("请勾选需要下载的文件!"));
	}
}


void CSynchronizeDlg::OnSwitch()
{
	// 清空
	m_Local.DeleteAllItems();
	m_Server.DeleteAllItems();

	if(!IsCode)
	{
		// 本地
		HTREEITEM m_TreeRoot = m_Local.InsertItem(_T("本地源码库"));//插入根节点
		ShowFile(_T("Code"), m_TreeRoot);//根目录进行遍历

		// 云端
		Type = 1;
		GetServerInfo();

		IsCode = TRUE;
	}
	else
	{
		HTREEITEM m_TreeRoot = m_Local.InsertItem(_T("本地文件库"));//插入根节点
		ShowFile(_T("File"), m_TreeRoot);//根目录进行遍历

		// 云端
		Type = 2;
		GetServerInfo();

		IsCode = FALSE;
	}
}


void CSynchronizeDlg::OnVersion()
{
	CWebProjectDlg dlg;
	dlg.UserId   = UserId;
	dlg.UserName = UserName;
	dlg.DoModal();
}


void CSynchronizeDlg::OnCancel()
{
	CDialogEx::OnCancel();
}
